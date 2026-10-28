#include <algorithm>
#include <memory>
#include <type_traits>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "huffman/BitReader.hpp"
#include "huffman/BitWriter.hpp"
#include "huffman/Compression.hpp"
#include "huffman/Decompression.hpp"
#include "huffman/FrequencyTable.hpp"
#include "huffman/HuffmanTree.hpp"
#include "huffman/HuffmanDecoder.hpp"
#include "huffman/UniquePtrPriorityQueue.hpp"

namespace
{

    namespace fs = std::filesystem;

    const fs::path TEST_OUTPUT_DIRECTORY{
        "build/test_output"};

    // -----------------------------------------------------------------------------
    // Minimal test framework
    // -----------------------------------------------------------------------------

    class TestFailure : public std::runtime_error
    {
    public:
        explicit TestFailure(const std::string &message)
            : std::runtime_error{message}
        {
        }
    };

    void expect(bool condition, const std::string &message)
    {
        if (!condition)
        {
            throw TestFailure{message};
        }
    }

    template <typename Actual, typename Expected>
    void expectEqual(
        const Actual &actual,
        const Expected &expected,
        const std::string &message)
    {
        if (!(actual == expected))
        {
            throw TestFailure{message};
        }
    }

    class TestSuite
    {
    public:
        explicit TestSuite(std::string name)
            : name_{std::move(name)}
        {
        }

        template <typename TestFunction>
        void run(const std::string &testName, TestFunction &&testFunction)
        {
            ++total_;

            try
            {
                testFunction();
                ++passed_;

                std::cout << "  [PASS] " << testName << '\n';
            }
            catch (const std::exception &exception)
            {
                std::cout
                    << "  [FAIL] " << testName
                    << "\n         " << exception.what()
                    << '\n';
            }
            catch (...)
            {
                std::cout
                    << "  [FAIL] " << testName
                    << "\n         Unknown exception"
                    << '\n';
            }
        }

        void printSummary() const
        {
            std::cout
                << '\n'
                << passed_ << '/' << total_
                << " test cases for " << name_ << " passed.\n";
        }

        [[nodiscard]] std::size_t passed() const
        {
            return passed_;
        }

        [[nodiscard]] std::size_t total() const
        {
            return total_;
        }

    private:
        std::string name_;
        std::size_t passed_{0};
        std::size_t total_{0};
    };

    // -----------------------------------------------------------------------------
    // Unique-pointer priority-queue tests
    // -----------------------------------------------------------------------------

    struct QueueTestNode
    {
        int frequency;
        std::unique_ptr<QueueTestNode> left;
        std::unique_ptr<QueueTestNode> right;
        int *liveCount;

        explicit QueueTestNode(int value, int *count = nullptr)
            : frequency{value}, liveCount{count}
        {
            if (liveCount)
            {
                ++*liveCount;
            }
        }

        ~QueueTestNode()
        {
            if (liveCount)
            {
                --*liveCount;
            }
        }
    };

    struct QueueTestCompare
    {
        bool minimumFirst{true};

        bool operator()(const QueueTestNode &a, const QueueTestNode &b) const noexcept
        {
            return minimumFirst ? a.frequency > b.frequency
                                : a.frequency < b.frequency;
        }
    };

    using TestNodeQueue = UniquePtrPriorityQueue<QueueTestNode, QueueTestCompare>;

    static_assert(!std::is_copy_constructible_v<TestNodeQueue>);
    static_assert(!std::is_copy_assignable_v<TestNodeQueue>);
    static_assert(std::is_move_constructible_v<TestNodeQueue>);
    static_assert(std::is_move_assignable_v<TestNodeQueue>);

    template <typename Exception, typename Function>
    void expectQueueException(Function &&function, const std::string &message)
    {
        bool caught{false};
        try
        {
            function();
        }
        catch (const Exception &)
        {
            caught = true;
        }
        expect(caught, message);
    }

    TestSuite runUniquePtrPriorityQueueTests()
    {
        TestSuite suite{"Unique Pointer Priority Queue"};
        std::cout << "\n=== Unique Pointer Priority Queue ===\n";

        suite.run("new queue reports its size and capacity", []
                  {
            TestNodeQueue queue{3};
            expect(queue.empty(), "New queue should be empty");
            expect(!queue.full(), "New positive-capacity queue should not be full");
            expectEqual(queue.size(), std::size_t{0}, "Initial size is incorrect");
            expectEqual(queue.capacity(), std::size_t{3}, "Capacity is incorrect"); });

        suite.run("push and extraction transfer the same node", []
                  {
            TestNodeQueue queue{1};
            auto node = std::make_unique<QueueTestNode>(7);
            const auto *address = node.get();
            queue.push(std::move(node));
            expect(!node, "Successful push should transfer ownership");
            expectEqual(queue.size(), std::size_t{1}, "Push should increase size");
            expect(queue.full(), "One-slot queue should be full");
            auto result = queue.extract_top();
            expectEqual(result.get(), address, "Extraction must preserve object identity");
            expect(queue.empty(), "Extraction should empty the queue");
            expectEqual(queue.size(), std::size_t{0}, "Extraction should decrease size");
            expect(!queue.full(), "Extracted queue should no longer be full"); });

        suite.run("mixed frequencies are extracted in ascending order", []
                  {
            TestNodeQueue queue{6};
            for (int frequency : {45, 13, 12, 16, 9, 5})
            {
                queue.push(std::make_unique<QueueTestNode>(frequency));
            }
            for (int expected : {5, 9, 12, 13, 16, 45})
            {
                const auto node = queue.extract_top();
                expectEqual(node->frequency, expected, "Minimum-frequency ordering is incorrect");
            }
            expect(queue.empty(), "All nodes should have been extracted"); });

        suite.run("equal priorities preserve every distinct node", []
                  {
            TestNodeQueue queue{3};
            std::vector<const QueueTestNode *> addresses;
            for (int i{0}; i < 3; ++i)
            {
                auto node = std::make_unique<QueueTestNode>(5);
                addresses.push_back(node.get());
                queue.push(std::move(node));
            }
            // Keep extracted nodes alive; no ordering between ties is required.
            std::vector<std::unique_ptr<QueueTestNode>> extracted;
            while (!queue.empty())
            {
                auto node = queue.extract_top();
                const auto found = std::find(addresses.begin(), addresses.end(), node.get());
                expect(found != addresses.end(), "Duplicate or unexpected node extracted");
                addresses.erase(found);
                extracted.push_back(std::move(node));
            }
            expect(addresses.empty(), "A tied node was lost"); });

        suite.run("interleaved pushes and extractions maintain ordering", []
                  {
            TestNodeQueue queue{3};
            queue.push(std::make_unique<QueueTestNode>(8));
            queue.push(std::make_unique<QueueTestNode>(3));
            expectEqual(queue.extract_top()->frequency, 3, "Expected initial minimum");
            queue.push(std::make_unique<QueueTestNode>(1));
            queue.push(std::make_unique<QueueTestNode>(5));
            for (int expected : {1, 5, 8})
            {
                expectEqual(queue.extract_top()->frequency, expected, "Interleaved ordering is incorrect");
            } });

        suite.run("full queue rejects insertion without consuming ownership", []
                  {
            TestNodeQueue queue{1};
            queue.push(std::make_unique<QueueTestNode>(4));
            auto rejected = std::make_unique<QueueTestNode>(2);
            const auto *address = rejected.get();
            expectQueueException<std::length_error>(
                [&] { queue.push(std::move(rejected)); }, "Expected capacity exception");
            expectEqual(rejected.get(), address, "Rejected node should remain with caller");
            expectEqual(queue.size(), std::size_t{1}, "Rejected insertion changed size");
            expectEqual(queue.extract_top()->frequency, 4, "Rejected insertion changed queue contents"); });

        suite.run("null insertion is rejected without changing the queue", []
                  {
            TestNodeQueue queue{2};
            queue.push(std::make_unique<QueueTestNode>(6));
            std::unique_ptr<QueueTestNode> node;
            expectQueueException<std::invalid_argument>(
                [&] { queue.push(std::move(node)); }, "Expected null-pointer exception");
            expectEqual(queue.size(), std::size_t{1}, "Null insertion changed size");
            expectEqual(queue.extract_top()->frequency, 6, "Null insertion changed contents"); });

        suite.run("empty extraction throws and the queue remains reusable", []
                  {
            TestNodeQueue queue{1};
            expectQueueException<std::length_error>(
                [&] { auto node = queue.extract_top(); }, "Expected empty-queue exception");
            expect(queue.empty(), "Failed extraction changed empty state");
            queue.push(std::make_unique<QueueTestNode>(9));
            expectEqual(queue.extract_top()->frequency, 9, "Queue should work after failed extraction"); });

        suite.run("zero capacity rejects insertion and extraction", []
                  {
            TestNodeQueue queue{0};
            expect(queue.empty() && queue.full(), "Zero-capacity queue should be empty and full");
            expectEqual(queue.capacity(), std::size_t{0}, "Expected zero capacity");
            auto node = std::make_unique<QueueTestNode>(1);
            expectQueueException<std::length_error>(
                [&] { queue.push(std::move(node)); }, "Zero capacity should reject insertion");
            expect(static_cast<bool>(node), "Rejected insertion consumed ownership");
            expectQueueException<std::length_error>(
                [&] { auto result = queue.extract_top(); }, "Empty extraction should throw"); });

        suite.run("capacity is reusable after draining", []
                  {
            TestNodeQueue queue{2};
            for (int round{0}; round < 4; ++round)
            {
                queue.push(std::make_unique<QueueTestNode>(2));
                queue.push(std::make_unique<QueueTestNode>(1));
                expect(queue.full(), "Refilled queue should be full");
                expectEqual(queue.extract_top()->frequency, 1, "Refill minimum is incorrect");
                expectEqual(queue.extract_top()->frequency, 2, "Refill maximum is incorrect");
                expect(queue.empty(), "Drained queue should be empty");
                expectEqual(queue.capacity(), std::size_t{2}, "Draining changed capacity");
            } });

        suite.run("stateful comparator can select maximum first", []
                  {
            TestNodeQueue queue{3, QueueTestCompare{false}};
            for (int frequency : {2, 7, 1})
            {
                queue.push(std::make_unique<QueueTestNode>(frequency));
            }
            for (int expected : {7, 2, 1})
            {
                expectEqual(queue.extract_top()->frequency, expected, "Custom comparator was ignored");
            } });

        suite.run("default comparator works with integer pointees", []
                  {
            UniquePtrPriorityQueue<int> queue{3};
            for (int value : {2, 7, 1})
            {
                queue.push(std::make_unique<int>(value));
            }
            for (int expected : {7, 2, 1})
            {
                expectEqual(*queue.extract_top(), expected, "Default comparator should give maximum first");
            } });

        suite.run("move construction transfers nodes and comparator state", []
                  {
            TestNodeQueue source{3, QueueTestCompare{false}};
            source.push(std::make_unique<QueueTestNode>(2));
            source.push(std::make_unique<QueueTestNode>(8));
            TestNodeQueue destination{std::move(source)};
            expectEqual(destination.size(), std::size_t{2}, "Move lost nodes");
            expectEqual(destination.capacity(), std::size_t{3}, "Move lost capacity");
            destination.push(std::make_unique<QueueTestNode>(5));
            for (int expected : {8, 5, 2})
            {
                expectEqual(destination.extract_top()->frequency, expected, "Move lost comparator or ordering");
            } });

        suite.run("move assignment releases old nodes and transfers ownership", []
                  {
            int liveCount{0};
            {
                TestNodeQueue source{2, QueueTestCompare{false}};
                source.push(std::make_unique<QueueTestNode>(3, &liveCount));
                source.push(std::make_unique<QueueTestNode>(9, &liveCount));
                TestNodeQueue destination{1};
                destination.push(std::make_unique<QueueTestNode>(99, &liveCount));
                destination = std::move(source);
                expectEqual(liveCount, 2, "Move assignment should destroy the old destination node");
                expectEqual(destination.capacity(), std::size_t{2}, "Move assignment lost capacity");
                expectEqual(destination.size(), std::size_t{2}, "Move assignment lost nodes");
                expectEqual(destination.extract_top()->frequency, 9, "Move assignment lost comparator state");
                expectEqual(destination.extract_top()->frequency, 3, "Move assignment lost remaining node");
            }
            expectEqual(liveCount, 0, "Move assignment leaked nodes"); });

        suite.run("extracted nodes outlive the queue and queued nodes are destroyed", []
                  {
            int liveCount{0};
            std::unique_ptr<QueueTestNode> extracted;
            {
                TestNodeQueue queue{2};
                queue.push(std::make_unique<QueueTestNode>(1, &liveCount));
                queue.push(std::make_unique<QueueTestNode>(2, &liveCount));
                extracted = queue.extract_top();
                expectEqual(liveCount, 2, "Extraction should not destroy either node");
            }
            expectEqual(liveCount, 1, "Queue destruction should release only its remaining node");
            expectEqual(extracted->frequency, 1, "Extracted node should remain valid");
            extracted.reset();
            expectEqual(liveCount, 0, "Extracted node should be destroyed by its new owner"); });

        suite.run("Huffman merges preserve subtrees and produce expected cost", []
                  {
            int liveCount{0};
            {
                TestNodeQueue queue{6};
                for (int frequency : {45, 13, 12, 16, 9, 5})
                {
                    queue.push(std::make_unique<QueueTestNode>(frequency, &liveCount));
                }
                int mergeCost{0};
                while (queue.size() > 1)
                {
                    auto left = queue.extract_top();
                    auto right = queue.extract_top();
                    auto parent = std::make_unique<QueueTestNode>(
                        left->frequency + right->frequency, &liveCount);
                    mergeCost += parent->frequency;
                    parent->left = std::move(left);
                    parent->right = std::move(right);
                    queue.push(std::move(parent));
                }
                auto root = queue.extract_top();
                expectEqual(root->frequency, 100, "Root should contain total frequency");
                expectEqual(mergeCost, 224, "Expected optimal Huffman merge cost");
                expectEqual(liveCount, 11, "All six leaves and five parents must remain alive");
                expect(root->left && root->right, "Root should own both subtrees");
                expect(queue.empty(), "Final extraction should empty the queue");
            }
            expectEqual(liveCount, 0, "Huffman subtree ownership leaked nodes"); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // File helpers
    // -----------------------------------------------------------------------------

    void writeTestFile(
        const fs::path &filename,
        const std::string &contents)
    {
        std::ofstream output{
            filename,
            std::ios::binary | std::ios::trunc};

        expect(
            output.is_open(),
            "Could not create test file: " + filename.string());

        output.write(
            contents.data(),
            static_cast<std::streamsize>(contents.size()));

        expect(
            output.good(),
            "Could not write test file: " + filename.string());
    }

    std::vector<uint8_t> readFileBytes(const fs::path &filename)
    {
        std::ifstream input{filename, std::ios::binary};

        expect(
            input.is_open(),
            "Could not open output file: " + filename.string());

        std::vector<uint8_t> bytes;
        char rawByte{};

        while (input.get(rawByte))
        {
            bytes.push_back(
                static_cast<uint8_t>(
                    static_cast<unsigned char>(rawByte)));
        }

        return bytes;
    }

    std::array<uint64_t, 256> countFrequencies(
        const std::string &testName,
        const std::string &contents)
    {
        const fs::path filename{
            TEST_OUTPUT_DIRECTORY / (testName + ".txt")};

        writeTestFile(filename, contents);

        return findFrequencies(filename.string());
    }

    std::size_t countNonZeroFrequencies(
        const std::array<uint64_t, 256> &frequencies)
    {
        std::size_t count{0};

        for (uint64_t frequency : frequencies)
        {
            if (frequency != 0)
            {
                ++count;
            }
        }

        return count;
    }

    uint64_t frequencyOf(
        const std::array<uint64_t, 256> &frequencies,
        unsigned char character)
    {
        return frequencies[static_cast<std::size_t>(character)];
    }

    template <typename WriteFunction>
    std::vector<uint8_t> writeWithBitWriter(
        const std::string &testName,
        WriteFunction &&writeFunction)
    {
        const fs::path filename{
            TEST_OUTPUT_DIRECTORY / (testName + ".bin")};

        {
            std::ofstream output{
                filename,
                std::ios::binary | std::ios::trunc};

            expect(
                output.is_open(),
                "Could not create BitWriter output file");

            BitWriter writer{output};
            writeFunction(writer);
            writer.flush();
        }

        return readFileBytes(filename);
    }

    std::vector<uint8_t> writeBits(
        const std::string &testName,
        const std::vector<uint8_t> &bits)
    {
        return writeWithBitWriter(
            testName,
            [&bits](BitWriter &writer)
            {
                for (uint8_t bit : bits)
                {
                    writer.writeBit(bit);
                }
            });
    }

    std::vector<uint8_t> serializeTree(
        const std::string &testName,
        std::array<uint64_t, 256> frequencies)
    {
        return writeWithBitWriter(
            testName,
            [&frequencies](BitWriter &writer)
            {
                HuffmanTree tree{frequencies};
                tree.serialize(writer);
            });
    }

    uint64_t readNativeUint64(
        const std::vector<uint8_t> &bytes,
        std::size_t offset)
    {
        expect(
            bytes.size() >= offset + sizeof(uint64_t),
            "Not enough bytes to read uint64_t field");

        uint64_t value{0};
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    void expectCompressionHeader(
        const std::vector<uint8_t> &bytes,
        uint64_t expectedOriginalSize)
    {
        constexpr std::size_t headerSize{
            4 + sizeof(uint8_t) + sizeof(uint64_t)};

        expect(
            bytes.size() >= headerSize,
            "Compressed output is smaller than the fixed header");

        expectEqual(bytes[0], uint8_t{'H'}, "Missing H in HUFF magic bytes");
        expectEqual(bytes[1], uint8_t{'U'}, "Missing U in HUFF magic bytes");
        expectEqual(bytes[2], uint8_t{'F'}, "Missing first F in HUFF magic bytes");
        expectEqual(bytes[3], uint8_t{'F'}, "Missing second F in HUFF magic bytes");
        expectEqual(bytes[4], uint8_t{1}, "Unexpected compressor format version");

        expectEqual(
            readNativeUint64(bytes, 5),
            expectedOriginalSize,
            "Original-size field in compressed header is incorrect");
    }

    std::vector<uint8_t> compressContents(
        const std::string &testName,
        const std::string &contents)
    {
        const fs::path inputPath{
            TEST_OUTPUT_DIRECTORY / (testName + ".txt")};
        const fs::path outputPath{
            TEST_OUTPUT_DIRECTORY / (testName + ".huff")};

        writeTestFile(inputPath, contents);
        compressor(inputPath.string(), outputPath.string());

        return readFileBytes(outputPath);
    }

    // -----------------------------------------------------------------------------
    // Huffman helpers
    // -----------------------------------------------------------------------------

    bool hasEncoding(const HuffmanTree::Encoding &encoding)
    {
        return encoding.size != 0;
    }

    bool fitsDeclaredSize(const HuffmanTree::Encoding &encoding)
    {
        if (encoding.size > 64)
        {
            return false;
        }

        if (encoding.size == 64)
        {
            return true;
        }

        return encoding.encoding < (uint64_t{1} << encoding.size);
    }

    bool isPrefixOf(
        const HuffmanTree::Encoding &possiblePrefix,
        const HuffmanTree::Encoding &value)
    {
        if (!hasEncoding(possiblePrefix) ||
            !hasEncoding(value) ||
            possiblePrefix.size > value.size)
        {
            return false;
        }

        const uint64_t trailingBits{
            value.size - possiblePrefix.size};

        return (value.encoding >> trailingBits) ==
               possiblePrefix.encoding;
    }

    std::size_t countDefinedEncodings(
        const std::array<HuffmanTree::Encoding, 256> &encodings)
    {
        std::size_t count{0};

        for (const auto &encoding : encodings)
        {
            if (hasEncoding(encoding))
            {
                ++count;
            }
        }

        return count;
    }

    bool isPrefixFree(
        const std::array<HuffmanTree::Encoding, 256> &encodings)
    {
        for (std::size_t i{0}; i < encodings.size(); ++i)
        {
            if (!hasEncoding(encodings[i]))
            {
                continue;
            }

            for (std::size_t j{0}; j < encodings.size(); ++j)
            {
                if (i == j || !hasEncoding(encodings[j]))
                {
                    continue;
                }

                if (isPrefixOf(encodings[i], encodings[j]))
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::array<uint64_t, 256> classicFrequencies()
    {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('a')] = 45;
        frequencies[static_cast<unsigned char>('b')] = 13;
        frequencies[static_cast<unsigned char>('c')] = 12;
        frequencies[static_cast<unsigned char>('d')] = 16;
        frequencies[static_cast<unsigned char>('e')] = 9;
        frequencies[static_cast<unsigned char>('f')] = 5;
        return frequencies;
    }

    // -----------------------------------------------------------------------------
    // Frequency-counting tests
    // -----------------------------------------------------------------------------

    TestSuite runFrequencyTests()
    {
        TestSuite suite{"Frequency Counting"};

        std::cout << "\n=== Frequency Counting ===\n";

        suite.run("empty file produces an empty table", []
                  {
        const auto frequencies{
            countFrequencies("frequency_empty", "")
        };

        expectEqual(
            countNonZeroFrequencies(frequencies),
            std::size_t{0},
            "Expected every frequency to be zero for an empty file"
        ); });

        suite.run("single character is counted once", []
                  {
        const auto frequencies{
            countFrequencies("frequency_single", "x")
        };

        expectEqual(
            countNonZeroFrequencies(frequencies),
            std::size_t{1},
            "Expected exactly one unique character"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('x')),
            uint64_t{1},
            "Expected x to have frequency 1"
        ); });

        suite.run("repeated character is counted correctly", []
                  {
        const auto frequencies{
            countFrequencies("frequency_repeated", "aaaaaaa")
        };

        expectEqual(
            countNonZeroFrequencies(frequencies),
            std::size_t{1},
            "Expected exactly one unique character"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('a')),
            uint64_t{7},
            "Expected a to have frequency 7"
        ); });

        suite.run("multiple character frequencies are correct", []
                  {
        const auto frequencies{
            countFrequencies("frequency_multiple", "aaabbc")
        };

        expectEqual(
            countNonZeroFrequencies(frequencies),
            std::size_t{3},
            "Expected three unique characters"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('a')),
            uint64_t{3},
            "Expected a to have frequency 3"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('b')),
            uint64_t{2},
            "Expected b to have frequency 2"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('c')),
            uint64_t{1},
            "Expected c to have frequency 1"
        ); });

        suite.run("spaces and tabs are counted", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_whitespace",
                "a a\tb"
            )
        };

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('a')),
            uint64_t{2},
            "Expected a to have frequency 2"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>(' ')),
            uint64_t{1},
            "Expected one space"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('\t')),
            uint64_t{1},
            "Expected one tab"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('b')),
            uint64_t{1},
            "Expected b to have frequency 1"
        ); });

        suite.run("uppercase, digits, and punctuation are distinct", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_punctuation",
                "A1!A1?"
            )
        };

        expectEqual(
            countNonZeroFrequencies(frequencies),
            std::size_t{4},
            "Expected four unique characters"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('A')),
            uint64_t{2},
            "Expected A to have frequency 2"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('1')),
            uint64_t{2},
            "Expected 1 to have frequency 2"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('!')),
            uint64_t{1},
            "Expected ! to have frequency 1"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('?')),
            uint64_t{1},
            "Expected ? to have frequency 1"
        ); });

        suite.run("newline characters are preserved and counted", []
                  {
        const auto frequencies{
            countFrequencies(
                "frequency_newlines",
                "a\nb\n"
            )
        };

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('a')),
            uint64_t{1},
            "Expected a to have frequency 1"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('b')),
            uint64_t{1},
            "Expected b to have frequency 1"
        );

        expect(
            frequencyOf(frequencies, static_cast<unsigned char>('\n')) != 0,
            "Newline character was removed from the input"
        );

        expectEqual(
            frequencyOf(frequencies, static_cast<unsigned char>('\n')),
            uint64_t{2},
            "Expected two newline characters"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Huffman-encoding tests
    // -----------------------------------------------------------------------------

    TestSuite runHuffmanEncodingTests()
    {
        TestSuite suite{"Huffman Encodings"};

        std::cout << "\n=== Huffman Encodings ===\n";

        suite.run("two symbols both receive encodings", []
                  {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('a')] = 1;
        frequencies[static_cast<unsigned char>('b')] = 1;

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            countDefinedEncodings(encodings),
            std::size_t{2},
            "Expected two encodings"
        );

        expect(
            hasEncoding(encodings[static_cast<unsigned char>('a')]),
            "Missing encoding for a"
        );

        expect(
            hasEncoding(encodings[static_cast<unsigned char>('b')]),
            "Missing encoding for b"
        ); });

        suite.run("encodings fit within their declared bit lengths", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        for (const auto &encoding : encodings)
        {
            if (!hasEncoding(encoding))
            {
                continue;
            }

            expect(
                fitsDeclaredSize(encoding),
                "An encoding uses bits outside its declared size"
            );
        } });

        suite.run("encodings are prefix-free", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            isPrefixFree(encodings),
            "One Huffman encoding is a prefix of another"
        ); });

        suite.run("zero-frequency symbols are ignored", []
                  {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('a')] = 3;
        frequencies[static_cast<unsigned char>('b')] = 2;

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            countDefinedEncodings(encodings),
            std::size_t{2},
            "Expected only positive-frequency symbols"
        );

        expect(
            !hasEncoding(encodings[static_cast<unsigned char>('z')]),
            "Zero-frequency symbol z should not receive an encoding"
        ); });

        suite.run("common symbols are not longer than rare symbols", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expect(
            encodings[static_cast<unsigned char>('a')].size <=
                encodings[static_cast<unsigned char>('f')].size,
            "Most-common symbol received a longer code than rarest symbol"
        ); });

        suite.run("classic frequencies have optimal weighted cost", []
                  {
        auto frequencies{classicFrequencies()};

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        uint64_t weightedCost{0};

        for (std::size_t i{0}; i < frequencies.size(); ++i)
        {
            const uint64_t frequency{frequencies[i]};

            if (frequency == 0)
            {
                continue;
            }

            weightedCost += frequency * encodings[i].size;
        }

        expectEqual(
            weightedCost,
            uint64_t{224},
            "Unexpected weighted encoding cost"
        ); });

        suite.run("single-symbol input receives a usable code", []
                  {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('x')] = 10;

        HuffmanTree tree{frequencies};
        const auto encodings{tree.getEncodings()};

        expectEqual(
            countDefinedEncodings(encodings),
            std::size_t{1},
            "Expected exactly one encoding"
        );

        expect(
            hasEncoding(encodings[static_cast<unsigned char>('x')]),
            "Missing encoding for x"
        );

        const auto &encoding{
            encodings[static_cast<unsigned char>('x')]
        };

        expectEqual(
            encoding.size,
            uint64_t{1},
            "Single-symbol encoding must contain one bit"
        );

        expectEqual(
            encoding.encoding,
            uint64_t{0},
            "Single-symbol encoding should be the bit 0"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Huffman-serialization tests
    // -----------------------------------------------------------------------------

    TestSuite runHuffmanSerializationTests()
    {
        TestSuite suite{"Huffman Serialization"};

        std::cout << "\n=== Huffman Serialization ===\n";

        suite.run("empty tree serializes to no bytes", []
                  {
        std::array<uint64_t, 256> frequencies{};

        const auto bytes{
            serializeTree("serialize_empty", frequencies)
        };

        expect(
            bytes.empty(),
            "Expected an empty Huffman tree to serialize to no bytes"
        ); });

        suite.run("single leaf writes marker followed by unaligned character byte", []
                  {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('a')] = 3;

        const auto bytes{
            serializeTree("serialize_single_leaf", frequencies)
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected serialized bits 1|01100001, padded to B0 80"
        ); });

        suite.run("two-leaf tree serializes in preorder", []
                  {
        std::array<uint64_t, 256> frequencies{};
        frequencies[static_cast<unsigned char>('a')] = 1;
        frequencies[static_cast<unsigned char>('b')] = 2;

        const auto bytes{
            serializeTree("serialize_two_leaf", frequencies)
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x58, 0x6C, 0x40},
            "Expected preorder serialization 0|1a|1b"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // BitWriter tests
    // -----------------------------------------------------------------------------

    TestSuite runBitWriterTests()
    {
        TestSuite suite{"BitWriter"};

        std::cout << "\n=== BitWriter ===\n";

        suite.run("flushing without bits creates an empty file", []
                  {
        const auto bytes{
            writeBits("bitwriter_empty", {})
        };

        expect(
            bytes.empty(),
            "Expected an empty output file"
        ); });

        suite.run("eight zero bits produce 0x00", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_zero_byte",
                {0, 0, 0, 0, 0, 0, 0, 0}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x00},
            "Expected exactly one 0x00 byte"
        ); });

        suite.run("eight one bits produce 0xFF", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_one_byte",
                {1, 1, 1, 1, 1, 1, 1, 1}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xFF},
            "Expected exactly one 0xFF byte"
        ); });

        suite.run("11110000 produces 0xF0", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_mixed_byte",
                {1, 1, 1, 1, 0, 0, 0, 0}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xF0},
            "Expected exactly one 0xF0 byte"
        ); });

        suite.run("multiple complete bytes are written correctly", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_multiple_bytes",
                {
                    1, 1, 1, 1, 0, 0, 0, 0,
                    1, 0, 1, 0, 1, 0, 1, 0
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xF0, 0xAA},
            "Expected bytes 0xF0 and 0xAA"
        ); });

        suite.run("partial byte is padded on the right", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_partial_byte",
                {1, 0, 1}
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xA0},
            "Expected partial byte 101 to become 10100000"
        ); });

        suite.run("bits crossing a byte boundary are preserved", []
                  {
        const auto bytes{
            writeBits(
                "bitwriter_cross_boundary",
                {
                    1, 0, 1, 0, 1, 0, 1, 0,
                    1
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xAA, 0x80},
            "Expected bytes 0xAA and 0x80"
        ); });

        suite.run("writeBits with zero length writes nothing", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_empty",
                [](BitWriter &writer)
                {
                    writer.writeBits(0, 0);
                }
            )
        };

        expect(
            bytes.empty(),
            "A zero-length encoding should not write any bits"
        ); });

        suite.run("writeBits preserves leading zeroes", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_leading_zeroes",
                [](BitWriter &writer)
                {
                    // Integer 1 with size 3 represents the encoding 001.
                    writer.writeBits(0b001, 3);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x20},
            "Expected encoding 001 to be padded to 00100000"
        ); });

        suite.run("writeBits writes an aligned complete byte", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_aligned_byte",
                [](BitWriter &writer)
                {
                    writer.writeBits(0xA5, 8);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xA5},
            "Expected eight bulk-written bits to produce 0xA5"
        ); });

        suite.run("writeBits calls remain contiguous across a byte boundary", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_cross_boundary",
                [](BitWriter &writer)
                {
                    writer.writeBits(0b101, 3);
                    writer.writeBits(0b110011, 6);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB9, 0x80},
            "Expected contiguous stream 101110011 padded to B9 80"
        ); });

        suite.run("writeBits supports the full 64-bit input width", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_64",
                [](BitWriter &writer)
                {
                    writer.writeBits(
                        UINT64_C(0x0123456789ABCDEF),
                        64);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{
                0x01, 0x23, 0x45, 0x67,
                0x89, 0xAB, 0xCD, 0xEF
            },
            "Expected all 64 bits to be written most-significant byte first"
        ); });

        suite.run("writeBits crosses the output-buffer boundary", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_write_bits_buffer_boundary",
                [](BitWriter &writer)
                {
                    for (std::size_t i{0}; i < 4097; ++i)
                    {
                        writer.writeBits(0xA5, 8);
                    }
                }
            )
        };

        expectEqual(
            bytes.size(),
            std::size_t{4097},
            "Expected every byte on both sides of the buffer flush"
        );

        for (uint8_t byte : bytes)
        {
            expectEqual(
                byte,
                uint8_t{0xA5},
                "A byte was corrupted while crossing the buffer boundary"
            );
        } });

        suite.run("aligned writeByte writes the byte unchanged", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_aligned_byte",
                [](BitWriter &writer)
                {
                    writer.writeByte(0xA5);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xA5},
            "Expected aligned byte 0xA5"
        ); });

        suite.run("writeByte works after one pending bit", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_one_bit",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeByte(0x61);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected bit stream 1|01100001 to cross the byte boundary correctly"
        ); });

        suite.run("writeByte works after three pending bits", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_three_bits",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeBit(0);
                    writer.writeBit(1);
                    writer.writeByte(0xA5);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xB4, 0xA0},
            "Expected bit stream 101|10100101 to remain contiguous"
        ); });

        suite.run("bit writes continue at the correct offset after writeByte", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_unaligned_then_bit",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeByte(0x00);
                    writer.writeBit(1);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0x80, 0x40},
            "Expected trailing bit to follow the unaligned byte without a gap"
        ); });

        suite.run("multiple bytes can be written while unaligned", []
                  {
        const auto bytes{
            writeWithBitWriter(
                "bitwriter_multiple_unaligned_bytes",
                [](BitWriter &writer)
                {
                    writer.writeBit(1);
                    writer.writeBit(0);
                    writer.writeBit(1);
                    writer.writeByte(0xF0);
                    writer.writeByte(0x0F);
                }
            )
        };

        expectEqual(
            bytes,
            std::vector<uint8_t>{0xBE, 0x01, 0xE0},
            "Expected consecutive unaligned bytes to preserve every bit"
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // BitReader tests
    // -----------------------------------------------------------------------------

    std::istringstream makeBitReaderInput(const std::vector<uint8_t> &bytes)
    {
        return std::istringstream{
            std::string{bytes.begin(), bytes.end()},
            std::ios::in | std::ios::binary};
    }

    template <typename ReadFunction>
    void expectReadError(
        ReadFunction &&readFunction,
        const std::string &message)
    {
        try
        {
            readFunction();
        }
        catch (const TestFailure &)
        {
            throw;
        }
        catch (const std::runtime_error &)
        {
            return;
        }

        throw TestFailure{message};
    }

    TestSuite runBitReaderTests()
    {
        TestSuite suite{"BitReader"};

        std::cout << "\n=== BitReader ===\n";

        // Fixed byte fixtures check the reader independently of BitWriter.
        suite.run("readBit rejects empty input", []
                  {
            auto input{makeBitReaderInput({})};
            BitReader reader{input};

            expectReadError(
                [&reader] { (void)reader.readBit(); },
                "Reading an empty input should throw instead of producing a bit"); });

        suite.run("zero-length reads return zero without consuming bits", []
                  {
            auto emptyInput{makeBitReaderInput({})};
            BitReader emptyReader{emptyInput};
            expectEqual(emptyReader.readBits(0), uint64_t{0},
                        "A zero-length read should succeed even on empty input");

            auto input{makeBitReaderInput({0xA5})};
            BitReader reader{input};
            expectEqual(reader.readBits(0), uint64_t{0},
                        "A zero-length read before the first refill should return zero");
            expectEqual(reader.readBits(3), uint64_t{0b101},
                        "A zero-length read consumed the beginning of the input");
            expectEqual(reader.readBits(0), uint64_t{0},
                        "A zero-length read within a buffer should return zero");
            expectEqual(reader.readBits(5), uint64_t{0b00101},
                        "A zero-length read changed the current bit position");
            expectEqual(reader.readBits(0), uint64_t{0},
                        "A zero-length read at the end should return zero"); });

        suite.run("readBit reads most-significant bit first", []
                  {
            auto input{makeBitReaderInput({0xA5})};
            BitReader reader{input};
            const std::array<uint8_t, 8> expected{1, 0, 1, 0, 0, 1, 0, 1};

            for (uint8_t bit : expected)
            {
                expectEqual(reader.readBit(), bit,
                            "Expected 0xA5 to be read as 10100101");
            } });

        suite.run("readByte preserves all 256 byte values across refills", []
                  {
            std::vector<uint8_t> bytes;
            for (unsigned value{0}; value < 256; ++value)
            {
                bytes.push_back(static_cast<uint8_t>(value));
            }

            auto input{makeBitReaderInput(bytes)};
            BitReader reader{input};

            for (uint8_t expected : bytes)
            {
                expectEqual(reader.readByte(), expected,
                            "Byte value was corrupted: " + std::to_string(expected));
            }

            expectReadError(
                [&reader] { (void)reader.readByte(); },
                "Reading beyond the last complete buffer should throw"); });

        suite.run("readBits preserves leading zeroes and their positions", []
                  {
            auto input{makeBitReaderInput({0x20, 0xE0})};
            BitReader reader{input};

            expectEqual(reader.readBits(3), uint64_t{0b001},
                        "Expected the first three bits to represent 001");
            expectEqual(reader.readBits(5), uint64_t{0},
                        "Leading zeroes must count toward the number of consumed bits");
            expectEqual(reader.readBits(3), uint64_t{0b111},
                        "The next byte should begin at the correct position");
            expectEqual(reader.readBits(5), uint64_t{0},
                        "Expected the remaining five zero bits"); });

        suite.run("readBits calls remain contiguous across byte boundaries", []
                  {
            auto input{makeBitReaderInput({0xB9, 0x80})};
            BitReader reader{input};

            expectEqual(reader.readBits(3), uint64_t{0b101},
                        "Expected the first three bits to be 101");
            expectEqual(reader.readBits(6), uint64_t{0b110011},
                        "Expected the next six bits to cross the byte boundary");
            // The bit reader exposes padding stored in the last byte too.
            expectEqual(reader.readBits(7), uint64_t{0},
                        "Expected seven right-padding zero bits");
            expectReadError(
                [&reader] { (void)reader.readBit(); },
                "Only padding actually stored in the input may be read"); });

        suite.run("readBits supports every width from 1 through 64", []
                  {
            constexpr uint64_t word{UINT64_C(0xFEDCBA9876543210)};

            for (uint64_t width{1}; width <= 64; ++width)
            {
                auto input{makeBitReaderInput(
                    {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10})};
                BitReader reader{input};

                expectEqual(reader.readBits(width), word >> (64 - width),
                            "Incorrect result for bit width " + std::to_string(width));

                // Since width >= 1, remaining is at most 63: no shift by 64.
                const uint64_t remaining{64 - width};
                const uint64_t mask{(uint64_t{1} << remaining) - 1};
                expectEqual(reader.readBits(remaining), word & mask,
                            "A read consumed the wrong number of bits at width " +
                                std::to_string(width));
            } });

        suite.run("readBits supports zero and all-one 64-bit words", []
                  {
            for (uint8_t byte : std::array<uint8_t, 2>{0x00, 0xFF})
            {
                auto input{makeBitReaderInput(std::vector<uint8_t>(8, byte))};
                BitReader reader{input};
                const uint64_t expected{
                    byte == 0 ? uint64_t{0} : UINT64_MAX};

                expectEqual(reader.readBits(64), expected,
                            "A full-width read lost or introduced bits");
            } });

        suite.run("mixed bit byte and bulk reads share one cursor", []
                  {
            auto input{makeBitReaderInput({0xB4, 0xA6})};
            BitReader reader{input};

            expectEqual(reader.readBits(3), uint64_t{0b101},
                        "Expected the three-bit prefix");
            expectEqual(reader.readByte(), uint8_t{0xA5},
                        "readByte should read the next eight bits while unaligned");
            expectEqual(reader.readBit(), uint8_t{0},
                        "readBit should continue immediately after readByte");
            expectEqual(reader.readBits(4), uint64_t{0b0110},
                        "Mixed reads lost the four-bit suffix"); });

        suite.run("readByte decodes an unaligned leaf character", []
                  {
            auto input{makeBitReaderInput({0xB0, 0x80})};
            BitReader reader{input};

            expectEqual(reader.readBit(), uint8_t{1},
                        "Expected the Huffman leaf marker");
            expectEqual(reader.readByte(), uint8_t{'a'},
                        "Expected the unaligned character in 1|01100001");
            expectEqual(reader.readBits(7), uint64_t{0},
                        "Expected seven padding bits after the leaf character"); });

        suite.run("readBit advances correctly across the 64-bit boundary", []
                  {
            auto input{makeBitReaderInput(
                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80})};
            BitReader reader{input};

            expectEqual(reader.readBits(63), uint64_t{0},
                        "Expected 63 zero bits before the end of the first buffer");
            expectEqual(reader.readBit(), uint8_t{1},
                        "The last bit of the first buffer was skipped");
            expectEqual(reader.readBit(), uint8_t{1},
                        "The first bit of the new buffer was skipped");
            expectEqual(reader.readBits(7), uint64_t{0},
                        "Expected the remainder of the final byte"); });

        suite.run("readBits combines data from both sides of a refill", []
                  {
            auto input{makeBitReaderInput(
                {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF, 0xB4})};
            BitReader reader{input};

            expectEqual(reader.readBits(60), UINT64_C(0x0123456789ABCDE),
                        "Expected the first 60 bits in input order");
            expectEqual(reader.readBits(8), uint64_t{0xFB},
                        "Expected four old bits and four new bits to form 0xFB");
            expectEqual(reader.readBits(4), uint64_t{0x4},
                        "Crossing a refill consumed too many bits"); });

        suite.run("readByte can span the 64-bit refill boundary", []
                  {
            auto input{makeBitReaderInput(
                {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x96})};
            BitReader reader{input};

            expectEqual(reader.readBits(63), uint64_t{0},
                        "Expected the zero prefix");
            expectEqual(reader.readByte(), uint8_t{0xCB},
                        "Expected byte 1|1001011 to cross the refill boundary");
            expectEqual(reader.readBit(), uint8_t{0},
                        "Expected the last bit of 0x96 to remain unread"); });

        suite.run("a full 64-bit read can begin at an unaligned position", []
                  {
            // Bits: 1 | 0x0123456789ABCDEF | seven zero padding bits.
            auto input{makeBitReaderInput(
                {0x80, 0x91, 0xA2, 0xB3, 0xC4, 0xD5, 0xE6, 0xF7, 0x80})};
            BitReader reader{input};

            expectEqual(reader.readBit(), uint8_t{1},
                        "Expected the one-bit prefix");
            expectEqual(reader.readBits(64), UINT64_C(0x0123456789ABCDEF),
                        "An unaligned full-width read corrupted the word");
            expectEqual(reader.readBits(7), uint64_t{0},
                        "Expected padding after the full-width read"); });

        suite.run("partial refills expose only the bytes actually loaded", []
                  {
            // Cover every short refill length, initially and after a full buffer.
            for (std::size_t prefix : std::array<std::size_t, 2>{0, 8})
            {
                for (std::size_t tail{1}; tail < 8; ++tail)
                {
                    std::vector<uint8_t> bytes(prefix, 0xFF);
                    for (std::size_t i{0}; i < tail; ++i)
                    {
                        bytes.push_back(static_cast<uint8_t>(0x80 | (i * 13 + tail)));
                    }

                    auto input{makeBitReaderInput(bytes)};
                    BitReader reader{input};

                    if (prefix != 0)
                    {
                        expectEqual(reader.readBits(64), UINT64_MAX,
                                    "Expected the complete all-one prefix");
                    }

                    for (std::size_t i{prefix}; i < bytes.size(); ++i)
                    {
                        expectEqual(reader.readByte(), bytes[i],
                                    "Incorrect byte in a partial refill of length " +
                                        std::to_string(tail));
                    }

                    expectReadError(
                        [&reader] { (void)reader.readBit(); },
                        "A partial refill exposed bits beyond the actual input");
                }
            } });

        suite.run("EOF after exactly eight bytes throws on further reads", []
                  {
            auto input{makeBitReaderInput(
                {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF})};
            BitReader reader{input};

            expectEqual(reader.readBits(64), UINT64_C(0x0123456789ABCDEF),
                        "Expected the complete input word");
            expectReadError([&reader] { (void)reader.readBit(); },
                            "readBit should throw at EOF");
            expectReadError([&reader] { (void)reader.readByte(); },
                            "readByte should continue to throw after EOF");
            expectReadError([&reader] { (void)reader.readBits(64); },
                            "readBits should continue to throw after EOF");
            expectEqual(reader.readBits(0), uint64_t{0},
                        "A zero-length read should still succeed after EOF"); });

        suite.run("a final zero byte is data rather than EOF", []
                  {
            auto input{makeBitReaderInput({0xA5, 0x00})};
            BitReader reader{input};

            expectEqual(reader.readByte(), uint8_t{0xA5},
                        "Expected the first byte");
            expectEqual(reader.readByte(), uint8_t{0x00},
                        "The final zero byte was mistaken for EOF");
            expectReadError([&reader] { (void)reader.readBit(); },
                            "The input should end after the actual zero byte"); });

        suite.run("readBits throws if fewer bits remain than requested", []
                  {
            auto input{makeBitReaderInput({0xAB})};
            BitReader reader{input};

            expectReadError(
                [&reader] { (void)reader.readBits(9); },
                "An eight-bit input cannot satisfy a nine-bit request"); });

        suite.run("readByte throws when only part of a byte remains", []
                  {
            auto input{makeBitReaderInput({0xA0})};
            BitReader reader{input};

            expectEqual(reader.readBits(3), uint64_t{0b101},
                        "Expected the three-bit prefix");
            expectReadError(
                [&reader] { (void)reader.readByte(); },
                "Five remaining bits cannot satisfy a readByte request"); });

        suite.run("stream failures are reported instead of producing data", []
                  {
            for (auto state : {std::ios::badbit, std::ios::failbit})
            {
                auto input{makeBitReaderInput({0xA5})};
                input.setstate(state);
                BitReader reader{input};

                expectReadError(
                    [&reader] { (void)reader.readBit(); },
                    "A failed input stream should cause a read error");
            } });

        suite.run("binary file input handles a full and a partial refill", []
                  {
            const std::vector<uint8_t> bytes{
                0x00, 0x80, 0xFF, 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
            const fs::path filename{TEST_OUTPUT_DIRECTORY / "bitreader_binary_input.bin"};
            writeTestFile(filename, std::string{bytes.begin(), bytes.end()});

            std::ifstream input{filename, std::ios::binary};
            expect(input.is_open(), "Could not open the BitReader input fixture");
            BitReader reader{input};

            expectEqual(reader.readBits(64), UINT64_C(0x0080FF0123456789),
                        "The binary file's first eight bytes were corrupted");
            expectEqual(reader.readBits(24), UINT64_C(0xABCDEF),
                        "The final three-byte refill was corrupted");
            expectReadError([&reader] { (void)reader.readBit(); },
                            "Reading beyond the binary file should throw"); });

        suite.run("reading starts at the stream's current position", []
                  {
            auto input{makeBitReaderInput({'H', 'U', 'F', 'F', 0xB9, 0x80})};
            input.ignore(4);
            BitReader reader{input};

            expectEqual(reader.readBits(3), uint64_t{0b101},
                        "The reader should start after the already-consumed header");
            expectEqual(reader.readBits(6), uint64_t{0b110011},
                        "Reading after a header changed the payload bit order"); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Compressor tests
    // -----------------------------------------------------------------------------

    // -----------------------------------------------------------------------------
    // Huffman deserialization and symbol-decoding tests
    // -----------------------------------------------------------------------------

    template <typename Decoder = HuffmanTree>
    void expectDecodeRoundTrip(const std::string &message)
    {
        expect(!message.empty(), "Empty input has no serialized tree");
        std::array<uint64_t, 256> frequencies{};
        for (unsigned char ch : message)
        {
            ++frequencies[ch];
        }

        std::ostringstream output{std::ios::out | std::ios::binary};
        // Destroy the encoding tree before decoding: only serialized bytes survive.
        {
            HuffmanTree encoder{frequencies};
            const auto encodings = encoder.getEncodings();
            BitWriter writer{output};
            encoder.serialize(writer);
            for (unsigned char ch : message)
            {
                const auto &code = encodings[ch];
                writer.writeBits(code.encoding, code.size);
            }
            // An extra field detects under/over-consumption, including single leaves.
            writer.writeBits(0b10101, 5);
            writer.flush();
        }

        std::istringstream input{output.str(), std::ios::in | std::ios::binary};
        BitReader reader{input};
        Decoder decoder{reader};
        std::string decoded;
        decoded.reserve(message.size());
        for (std::size_t i = 0; i < message.size(); ++i)
        {
            decoded.push_back(decoder.decodeSymbol());
        }
        expectEqual(decoded, message, "Decoded bytes differ from original input");
        expectEqual(reader.readBits(5), uint64_t{0b10101},
                    "Decoder consumed the wrong number of payload bits");
    }

    TestSuite runHuffmanDecodingTests()
    {
        TestSuite suite{"Huffman Decoding"};
        std::cout << "\n=== Huffman Decoding ===\n";

        suite.run("fixed preorder fixture decodes left and nested right paths", []
                  {
            // Tree: internal(A, internal(B, C)); codes A=0, B=10, C=11.
            // Tree bits: 0|1 01000001|0|1 01000010|1 01000011 (29 bits).
            // Payload ABCA: 0|10|11|0; sentinel: 10101; no padding needed.
            // Fixed bytes deliberately avoid relying on the production serializer.
            auto input = makeBitReaderInput({0x50, 0x54, 0x2A, 0x1A, 0xD5});
            BitReader reader{input};
            const HuffmanTree decoder{reader};
            for (char expected : std::string{"ABCA"})
            {
                expectEqual(decoder.decodeSymbol(), expected,
                            "Fixed fixture decoded an incorrect symbol");
            }
            expectEqual(reader.readBits(5), uint64_t{0b10101},
                        "Tree/payload parsing lost the exact bit position"); });

        suite.run("one character consumes its one-bit code", []
                  { expectDecodeRoundTrip("A"); });
        suite.run("repeated single symbol consumes every code bit", []
                  { expectDecodeRoundTrip(std::string(1000, 'A')); });
        suite.run("single null-byte symbol decodes correctly", []
                  { expectDecodeRoundTrip(std::string(17, '\0')); });
        suite.run("single high-byte symbol decodes correctly", []
                  { expectDecodeRoundTrip(std::string(19, static_cast<char>(0xFF))); });
        suite.run("two-symbol message crosses unaligned tree boundary", []
                  { expectDecodeRoundTrip("ABABBAABABBA"); });
        suite.run("varying code lengths reset traversal for each symbol", []
                  { expectDecodeRoundTrip("BANANA BANDANA\n"); });
        suite.run("all 256 byte values survive decoding", []
                  {
            std::string message;
            for (int byte = 0; byte < 256; ++byte)
                message.push_back(static_cast<char>(byte));
            expectDecodeRoundTrip(message); });
        suite.run("large deterministic binary message crosses reader refills", []
                  {
            std::string message;
            uint32_t state{0x12345678};
            for (std::size_t i = 0; i < 200000; ++i)
            {
                state = state * UINT32_C(1664525) + UINT32_C(1013904223);
                message.push_back(static_cast<char>(state >> 24));
            }
            expectDecodeRoundTrip(message); });
        suite.run("skewed frequencies decode long and short paths", []
                  {
            std::string message;
            std::size_t previous{1}, count{1};
            for (int symbol = 0; symbol < 16; ++symbol)
            {
                message.append(count, static_cast<char>(symbol));
                const auto next = previous + count;
                previous = count;
                count = next;
            }
            expectDecodeRoundTrip(message); });
        suite.run("empty serialized input rejects reconstruction", []
                  {
            auto input = makeBitReaderInput({});
            BitReader reader{input};
            expectReadError([&] { HuffmanTree decoder{reader}; },
                            "Missing tree should throw"); });
        suite.run("truncated leaf character rejects reconstruction", []
                  {
            // Leaf marker plus only seven of the required eight character bits.
            auto input = makeBitReaderInput({0xA0});
            BitReader reader{input};
            expectReadError([&] { HuffmanTree decoder{reader}; },
                            "Incomplete leaf should throw"); });
        suite.run("internal node missing right subtree rejects reconstruction", []
                  {
            // 0 | 1 01000001 | 1 01000010 needs 19 bits; only 16 are present.
            auto input = makeBitReaderInput({0x50, 0x68});
            BitReader reader{input};
            expectReadError([&] { HuffmanTree decoder{reader}; },
                            "Incomplete right subtree should throw"); });
        suite.run("single-symbol decoder rejects a nonzero payload bit", []
                  {
            // 1 | 01000001 | 1, followed by padding.
            auto input = makeBitReaderInput({0xA0, 0xE0});
            BitReader reader{input};
            HuffmanTree decoder{reader};
            expectReadError([&] { (void)decoder.decodeSymbol(); },
                            "Single-symbol code must be zero"); });
        suite.run("decoder throws when payload is exhausted", []
                  {
            // Tree internal(A,B) is 19 bits; five remaining bits encode five As.
            auto input = makeBitReaderInput({0x50, 0x68, 0x40});
            BitReader reader{input};
            HuffmanTree decoder{reader};
            for (int i = 0; i < 5; ++i)
                expectEqual(decoder.decodeSymbol(), 'A', "Expected A before EOF");
            expectReadError([&] { (void)decoder.decodeSymbol(); },
                            "Reading beyond physical payload should throw"); });
        suite.run("frequency-built tree rejects decoding without an attached reader", []
                  {
            std::array<uint64_t, 256> frequencies{};
            frequencies['A'] = 1;
            HuffmanTree tree{frequencies};
            expectReadError([&] { (void)tree.decodeSymbol(); },
                            "Decoding without a reader should throw"); });

        suite.printSummary();
        return suite;
    }

    // Independent fixture writer: marker 0 = internal, 1 = leaf, MSB-first bytes.
    // Deliberately bypasses HuffmanTree::serialize and its encoding tables.
    void writeBalancedDecoderFixture(BitWriter &writer, unsigned first, unsigned leaves)
    {
        if (leaves == 1)
        {
            writer.writeBit(1);
            writer.writeByte(static_cast<uint8_t>(first));
            return;
        }
        writer.writeBit(0);
        const unsigned leftLeaves = leaves / 2;
        writeBalancedDecoderFixture(writer, first, leftLeaves);
        writeBalancedDecoderFixture(writer, first + leftLeaves, leaves - leftLeaves);
    }

    TestSuite runArrayHuffmanDecodingTests()
    {
        TestSuite suite{"Array Huffman Decoder"};
        std::cout << "\n=== Array Huffman Decoder ===\n";

        suite.run("fixed preorder fixture decodes left and nested right paths", []
                  {
            // Tree: internal(A, internal(B, C)); codes A=0, B=10, C=11.
            // Tree bits: 0|1 01000001|0|1 01000010|1 01000011 (29 bits).
            // Payload ABCA: 0|10|11|0; sentinel: 10101; no padding needed.
            // Fixed bytes deliberately avoid relying on the production serializer.
            auto input = makeBitReaderInput({0x50, 0x54, 0x2A, 0x1A, 0xD5});
            BitReader reader{input};
            const HuffmanDecoder decoder{reader};
            for (char expected : std::string{"ABCA"})
            {
                expectEqual(decoder.decodeSymbol(), expected,
                            "Fixed fixture decoded an incorrect symbol");
            }
            expectEqual(reader.readBits(5), uint64_t{0b10101},
                        "Tree/payload parsing lost the exact bit position"); });

        suite.run("one character consumes its one-bit code", []
                  { expectDecodeRoundTrip<HuffmanDecoder>("A"); });
        suite.run("repeated single symbol consumes every code bit", []
                  { expectDecodeRoundTrip<HuffmanDecoder>(std::string(1000, 'A')); });
        suite.run("single null-byte symbol decodes correctly", []
                  { expectDecodeRoundTrip<HuffmanDecoder>(std::string(17, '\0')); });
        suite.run("single high-byte symbol decodes correctly", []
                  { expectDecodeRoundTrip<HuffmanDecoder>(std::string(19, static_cast<char>(0xFF))); });
        suite.run("two-symbol message crosses unaligned tree boundary", []
                  { expectDecodeRoundTrip<HuffmanDecoder>("ABABBAABABBA"); });
        suite.run("varying code lengths reset traversal for each symbol", []
                  { expectDecodeRoundTrip<HuffmanDecoder>("BANANA BANDANA\n"); });
        suite.run("all 256 byte values survive decoding", []
                  {
            std::string message;
            for (int byte = 0; byte < 256; ++byte)
                message.push_back(static_cast<char>(byte));
            expectDecodeRoundTrip<HuffmanDecoder>(message); });
        suite.run("large deterministic binary message crosses reader refills", []
                  {
            std::string message;
            uint32_t state{0x12345678};
            for (std::size_t i = 0; i < 200000; ++i)
            {
                state = state * UINT32_C(1664525) + UINT32_C(1013904223);
                message.push_back(static_cast<char>(state >> 24));
            }
            expectDecodeRoundTrip<HuffmanDecoder>(message); });
        suite.run("skewed frequencies decode long and short paths", []
                  {
            std::string message;
            std::size_t previous{1}, count{1};
            for (int symbol = 0; symbol < 16; ++symbol)
            {
                message.append(count, static_cast<char>(symbol));
                const auto next = previous + count;
                previous = count;
                count = next;
            }
            expectDecodeRoundTrip<HuffmanDecoder>(message); });
        suite.run("empty serialized input rejects reconstruction", []
                  {
            auto input = makeBitReaderInput({});
            BitReader reader{input};
            expectReadError([&] { HuffmanDecoder decoder{reader}; },
                            "Missing tree should throw"); });
        suite.run("truncated leaf character rejects reconstruction", []
                  {
            // Leaf marker plus only seven of the required eight character bits.
            auto input = makeBitReaderInput({0xA0});
            BitReader reader{input};
            expectReadError([&] { HuffmanDecoder decoder{reader}; },
                            "Incomplete leaf should throw"); });
        suite.run("internal node missing right subtree rejects reconstruction", []
                  {
            // 0 | 1 01000001 | 1 01000010 needs 19 bits; only 16 are present.
            auto input = makeBitReaderInput({0x50, 0x68});
            BitReader reader{input};
            expectReadError([&] { HuffmanDecoder decoder{reader}; },
                            "Incomplete right subtree should throw"); });
        suite.run("single-symbol decoder rejects a nonzero payload bit", []
                  {
            // 1 | 01000001 | 1, followed by padding.
            auto input = makeBitReaderInput({0xA0, 0xE0});
            BitReader reader{input};
            HuffmanDecoder decoder{reader};
            expectReadError([&] { (void)decoder.decodeSymbol(); },
                            "Single-symbol code must be zero"); });
        suite.run("decoder throws when payload is exhausted", []
                  {
            // Tree internal(A,B) is 19 bits; five remaining bits encode five As.
            auto input = makeBitReaderInput({0x50, 0x68, 0x40});
            BitReader reader{input};
            HuffmanDecoder decoder{reader};
            for (int i = 0; i < 5; ++i)
                expectEqual(decoder.decodeSymbol(), 'A', "Expected A before EOF");
            expectReadError([&] { (void)decoder.decodeSymbol(); },
                            "Reading beyond physical payload should throw"); });

        suite.run("balanced 256-symbol tree uses all 511 node slots", []
                  {
            std::ostringstream output{std::ios::out | std::ios::binary};
            {
                BitWriter writer{output};
                writeBalancedDecoderFixture(writer, 0, 256);
                // Equal-depth leaves: the code for each byte is its eight-bit value.
                for (unsigned value = 0; value < 256; ++value)
                    writer.writeByte(static_cast<uint8_t>(value));
                for (int value = 255; value >= 0; --value)
                    writer.writeByte(static_cast<uint8_t>(value));
                writer.writeBits(0b10101, 5);
                writer.flush();
            }
            std::istringstream input{output.str(), std::ios::in | std::ios::binary};
            BitReader reader{input};
            const HuffmanDecoder decoder{reader};
            for (unsigned value = 0; value < 256; ++value)
                expectEqual(static_cast<unsigned char>(decoder.decodeSymbol()),
                            static_cast<unsigned char>(value), "Forward full-tree traversal failed");
            for (int value = 255; value >= 0; --value)
                expectEqual(static_cast<unsigned char>(decoder.decodeSymbol()),
                            static_cast<unsigned char>(value), "Reverse full-tree traversal failed");
            expectEqual(reader.readBits(5), uint64_t{0b10101},
                        "Full-tree decoding consumed the wrong number of bits"); });

        suite.run("511-node skewed tree supports paths up to 255 bits", []
                  {
            std::ostringstream output{std::ios::out | std::ios::binary};
            {
                BitWriter writer{output};
                // Each internal node has a left leaf and a right continuation.
                for (unsigned value = 0; value < 255; ++value)
                {
                    writer.writeBit(0);
                    writer.writeBit(1);
                    writer.writeByte(static_cast<uint8_t>(value));
                }
                writer.writeBit(1);
                writer.writeByte(0xFF);
                // Write bits individually: codes here can exceed uint64_t width.
                for (unsigned value = 0; value < 256; ++value)
                {
                    for (unsigned bit = 0; bit < value; ++bit)
                        writer.writeBit(1);
                    if (value != 255)
                        writer.writeBit(0);
                }
                writer.writeBits(0b10101, 5);
                writer.flush();
            }
            std::istringstream input{output.str(), std::ios::in | std::ios::binary};
            BitReader reader{input};
            const HuffmanDecoder decoder{reader};
            for (unsigned value = 0; value < 256; ++value)
                expectEqual(static_cast<unsigned char>(decoder.decodeSymbol()),
                            static_cast<unsigned char>(value), "Deep-path decoding failed");
            expectEqual(reader.readBits(5), uint64_t{0b10101},
                        "Deep-path decoding consumed the wrong number of bits"); });

        suite.run("513-node serialized tree is rejected instead of overflowing array", []
                  {
            std::ostringstream output{std::ios::out | std::ios::binary};
            {
                BitWriter writer{output};
                // Complete malformed tree: 257 leaves, 256 internal nodes.
                // One byte label repeats; this exceeds the byte-alphabet capacity.
                writeBalancedDecoderFixture(writer, 0, 257);
                writer.flush();
            }
            std::istringstream input{output.str(), std::ios::in | std::ios::binary};
            BitReader reader{input};
            expectReadError([&] { HuffmanDecoder decoder{reader}; },
                            "A tree exceeding 511 nodes must be rejected"); });

        suite.printSummary();
        return suite;
    }

    TestSuite runCompressorTests()
    {
        TestSuite suite{"Compressor"};

        std::cout << "\n=== Compressor ===\n";

        suite.run("empty input writes only the fixed header", []
                  {
        const auto bytes{
            compressContents("compress_empty", "")
        };

        expectCompressionHeader(bytes, 0);

        expectEqual(
            bytes.size(),
            std::size_t{4 + sizeof(uint8_t) + sizeof(uint64_t)},
            "Empty input should have no serialized tree or encoded payload"
        ); });

        suite.run("single-symbol input writes header tree and payload", []
                  {
        const auto bytes{
            compressContents("compress_single_symbol", "aaa")
        };

        expectCompressionHeader(bytes, 3);

        const std::vector<uint8_t> body{
            bytes.begin() + static_cast<std::ptrdiff_t>(
                4 + sizeof(uint8_t) + sizeof(uint64_t)),
            bytes.end()
        };

        expectEqual(
            body,
            std::vector<uint8_t>{0xB0, 0x80},
            "Expected serialized leaf 1|a followed by three 0 payload bits"
        ); });

        suite.run("two-symbol input keeps serialized tree and payload contiguous", []
                  {
        const auto bytes{
            compressContents("compress_two_symbols", "abb")
        };

        expectCompressionHeader(bytes, 3);

        const std::vector<uint8_t> body{
            bytes.begin() + static_cast<std::ptrdiff_t>(
                4 + sizeof(uint8_t) + sizeof(uint64_t)),
            bytes.end()
        };

        expectEqual(
            body,
            std::vector<uint8_t>{0x58, 0x6C, 0x4C},
            "Expected preorder tree 0|1a|1b followed immediately by payload 011"
        ); });

        suite.run("compressor records the exact original byte count", []
                  {
        const std::string contents{"a\n b\t!"};
        const auto bytes{
            compressContents("compress_original_size", contents)
        };

        expectCompressionHeader(
            bytes,
            static_cast<uint64_t>(contents.size())
        ); });

        suite.printSummary();
        return suite;
    }

    // -----------------------------------------------------------------------------
    // Full-file decompression tests (version 1 uses native-endian uint64_t size)
    // -----------------------------------------------------------------------------

    std::string makeDecompressionFixture(uint64_t originalSize,
                                         const std::vector<uint8_t> &body = {})
    {
        std::string bytes{"HUFF"};
        bytes.push_back('\1');
        bytes.append(reinterpret_cast<const char *>(&originalSize), sizeof(originalSize));
        for (uint8_t byte : body)
            bytes.push_back(static_cast<char>(byte));
        return bytes;
    }

    void expectFileRoundTrip(const std::string &name, const std::string &contents)
    {
        const auto original = TEST_OUTPUT_DIRECTORY / (name + ".original");
        const auto compressed = TEST_OUTPUT_DIRECTORY / (name + ".huff");
        const auto restored = TEST_OUTPUT_DIRECTORY / (name + ".restored");
        writeTestFile(original, contents);
        // A stale tail must be removed even for empty decoded output.
        writeTestFile(restored, std::string(contents.size() + 100, 'X'));
        compressor(original.string(), compressed.string());
        decompressor(compressed.string(), restored.string());
        expectEqual(readFileBytes(restored), readFileBytes(original),
                    "Full-file round trip changed bytes or left a stale output tail");
        expectEqual(fs::file_size(restored), static_cast<std::uintmax_t>(contents.size()),
                    "Decompressed file has an incorrect size");
    }

    void expectDecompressionFailure(const std::string &name, const std::string &bytes)
    {
        const auto input = TEST_OUTPUT_DIRECTORY / (name + ".huff");
        const auto output = TEST_OUTPUT_DIRECTORY / (name + ".restored");
        writeTestFile(input, bytes);
        expectReadError([&]
                        { decompressor(input.string(), output.string()); },
                        "Decompression should reject " + name);
    }

    TestSuite runDecompressorTests()
    {
        TestSuite suite{"Decompressor"};
        std::cout << "\n=== Decompressor ===\n";

        suite.run("empty file round trip creates an empty output", []
                  { expectFileRoundTrip("decompress_empty", ""); });
        suite.run("one character round trip ignores final padding", []
                  { expectFileRoundTrip("decompress_one", "A"); });
        suite.run("repeated single symbol round trip", []
                  { expectFileRoundTrip("decompress_repeated", std::string(10000, 'a')); });
        suite.run("two symbols round trip across unaligned tree boundary", []
                  { expectFileRoundTrip("decompress_two", "abb"); });
        suite.run("text preserves spaces tabs and line endings", []
                  { expectFileRoundTrip("decompress_text", "BANANA BANDANA\n\tline two\r\nlast line\r"); });
        suite.run("binary control characters and high bytes are preserved", []
                  {
            const char bytes[]{'\0', '\r', '\n', '\x1A', '\t',
                               static_cast<char>(0x80), static_cast<char>(0xFF), '\0'};
            expectFileRoundTrip("decompress_binary", std::string(bytes, sizeof(bytes))); });
        suite.run("all 256 byte values round trip", []
                  {
            std::string contents;
            for (int byte = 0; byte < 256; ++byte)
                contents.push_back(static_cast<char>(byte));
            expectFileRoundTrip("decompress_all_bytes", contents); });
        suite.run("round trips span compressor input block boundaries", []
                  {
            for (std::size_t size : {4095u, 4096u, 4097u, 65535u, 65536u, 65537u})
            {
                std::string contents;
                for (std::size_t i = 0; i < size; ++i)
                    contents.push_back(static_cast<char>((i * 37 + i / 256) & 255));
                expectFileRoundTrip("decompress_boundary_" + std::to_string(size), contents);
            } });
        suite.run("large deterministic binary file round trips", []
                  {
            std::string contents;
            uint32_t state{0xABCDEF01};
            for (std::size_t i = 0; i < 200000; ++i)
            {
                state = state * UINT32_C(1664525) + UINT32_C(1013904223);
                contents.push_back(static_cast<char>(state >> 24));
            }
            expectFileRoundTrip("decompress_large", contents); });
        suite.run("independent fixed file fixture decodes without compressor", []
                  {
            // Tree internal(a,b), codes a=0 b=1; payload abb=011, then padding.
            const auto input = TEST_OUTPUT_DIRECTORY / "decompress_fixed.huff";
            const auto output = TEST_OUTPUT_DIRECTORY / "decompress_fixed.restored";
            writeTestFile(input, makeDecompressionFixture(3, {0x58, 0x6C, 0x4C}));
            decompressor(input.string(), output.string());
            expectEqual(readFileBytes(output), std::vector<uint8_t>{'a', 'b', 'b'},
                        "Independent file fixture was decoded incorrectly"); });
        suite.run("every truncated fixed-header length is rejected", []
                  {
            const auto header = makeDecompressionFixture(0);
            for (std::size_t size = 0; size < header.size(); ++size)
                expectDecompressionFailure("decompress_short_header_" + std::to_string(size),
                                           header.substr(0, size)); });
        suite.run("each incorrect magic byte is rejected", []
                  {
            for (std::size_t i = 0; i < 4; ++i)
            {
                auto bytes = makeDecompressionFixture(0);
                bytes[i] = '?';
                expectDecompressionFailure("decompress_bad_magic_" + std::to_string(i), bytes);
            } });
        suite.run("unsupported file versions are rejected", []
                  {
            for (unsigned version : {0u, 2u, 255u})
            {
                auto bytes = makeDecompressionFixture(0);
                bytes[4] = static_cast<char>(version);
                expectDecompressionFailure("decompress_version_" + std::to_string(version), bytes);
            } });
        suite.run("nonempty original size requires a serialized tree", []
                  { expectDecompressionFailure("decompress_missing_tree", makeDecompressionFixture(1)); });
        suite.run("truncated serialized leaf is rejected", []
                  { expectDecompressionFailure("decompress_short_tree", makeDecompressionFixture(1, {0xA0})); });
        suite.run("physically truncated payload is rejected", []
                  {
            // Header requires 32 symbols; leaf A plus seven zero bits is insufficient.
            expectDecompressionFailure("decompress_short_payload",
                                       makeDecompressionFixture(32, {0xA0, 0x80})); });
        suite.run("invalid single-symbol payload bit is rejected", []
                  { expectDecompressionFailure("decompress_bad_bit", makeDecompressionFixture(1, {0xA0, 0xE0})); });
        suite.run("missing input file raises a runtime error", []
                  {
            const auto missing = TEST_OUTPUT_DIRECTORY / "decompress_missing_input.huff";
            const auto output = TEST_OUTPUT_DIRECTORY / "decompress_missing_input.restored";
            fs::remove(missing);
            expectReadError([&] { decompressor(missing.string(), output.string()); },
                            "Missing input should throw"); });
        suite.run("directory as output path raises a runtime error", []
                  {
            const auto input = TEST_OUTPUT_DIRECTORY / "decompress_output_error.huff";
            writeTestFile(input, makeDecompressionFixture(0));
            expectReadError([&] { decompressor(input.string(), TEST_OUTPUT_DIRECTORY.string()); },
                            "Opening a directory for output should throw"); });

        suite.printSummary();
        return suite;
    }

} // namespace

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main()
{
    try
    {
        fs::create_directories(TEST_OUTPUT_DIRECTORY);
    }
    catch (const fs::filesystem_error &exception)
    {
        std::cerr
            << "Could not create test-output directory: "
            << exception.what()
            << '\n';

        return 1;
    }

    std::cout
        << "============================================\n"
        << " Huffman Compression Lab - Naive Test Suite\n"
        << "============================================\n";

    const TestSuite queueSuite{
        runUniquePtrPriorityQueueTests()};

    const TestSuite frequencySuite{
        runFrequencyTests()};

    const TestSuite huffmanSuite{
        runHuffmanEncodingTests()};

    const TestSuite serializationSuite{
        runHuffmanSerializationTests()};

    const TestSuite bitWriterSuite{
        runBitWriterTests()};

    const TestSuite bitReaderSuite{
        runBitReaderTests()};

    const TestSuite decodingSuite{
        runHuffmanDecodingTests()};

    const TestSuite arrayDecodingSuite{
        runArrayHuffmanDecodingTests()};

    const TestSuite compressorSuite{
        runCompressorTests()};

    const TestSuite decompressorSuite{
        runDecompressorTests()};

    const std::size_t totalPassed{
        queueSuite.passed() +
        frequencySuite.passed() +
        huffmanSuite.passed() +
        serializationSuite.passed() +
        bitWriterSuite.passed() +
        bitReaderSuite.passed() +
        decodingSuite.passed() +
        arrayDecodingSuite.passed() +
        compressorSuite.passed() +
        decompressorSuite.passed()};

    const std::size_t totalTests{
        queueSuite.total() +
        frequencySuite.total() +
        huffmanSuite.total() +
        serializationSuite.total() +
        bitWriterSuite.total() +
        bitReaderSuite.total() +
        decodingSuite.total() +
        arrayDecodingSuite.total() +
        compressorSuite.total() +
        decompressorSuite.total()};

    std::cout
        << "\n============================================\n"
        << " Final Result: "
        << totalPassed << '/' << totalTests
        << " test cases passed\n"
        << "============================================\n";

    if (totalPassed == totalTests)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }

    std::cout << "SOME TESTS FAILED\n";
    return 1;
}