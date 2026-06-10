#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

int extractFileNaive(string pdfPath)
{
  string outputPath = "output.txt";
  string command = "pdftotext " + pdfPath + " " + outputPath;
  int status = system(command.c_str());
  // Check if the command executed successfully
  if (status == 0)
  {
    cout << "Text extraction successful." << endl;
  }
  else
  {
    cout << "Text extraction failed." << endl;
    return 1; // Exit the program with error code
  }
  // Open the output file to read the extracted text
  ifstream outputFile(outputPath);
  if (outputFile.is_open())
  {
    string textContent;
    string line;
    while (getline(outputFile, line))
    {
      textContent += line + "\n"; // Append each line to the textContent
    }
    outputFile.close();
  }
  else
  {
    cout << "Failed to open output file." << endl;
    return 1; // Exit the program with error code
  }
  return 0; // Exit the program successfully
}