#include <iostream>
#include <windows.h>
#include <string>
#include <fstream>

using namespace std;

HANDLE hCommDev;
DCB dcb;
LPCTSTR portName = "COM3";
COMMTIMEOUTS commTimeouts;

void printDcb(HANDLE hCommDev, DCB dcb)
{
    cout << "GetCommState: " << (GetCommState(hCommDev, &dcb) ? "SUCCESS" : "ERROR") << endl;
    cout << "BaudRate: " << dcb.BaudRate << endl;
    cout << "ByteSize: " << to_string(dcb.ByteSize) << endl;
    cout << "PARITY: ";
    switch (dcb.Parity)
    {
    case NOPARITY:
        cout << "NONE" << endl;
        break;
    case ODDPARITY:
        cout << "ODDPARITY" << endl;
        break;
    case EVENPARITY:
        cout << "EVENPARITY" << endl;
        break;
    case MARKPARITY:
        cout << "MARKPARITY" << endl;
    }
    cout << "STOP BITS: ";
    switch (dcb.StopBits)
    {
    case ONESTOPBIT:
        cout << "ONESTOPBIT" << endl;
        break;
    case TWOSTOPBITS:
        cout << "TWOSTOPBITS" << endl;
        break;
    case ONE5STOPBITS:
        cout << "ONE5STOPBITS" << endl;
    }
    cout << "DTR line state: ";
    switch (dcb.fDtrControl)
    {
    case DTR_CONTROL_DISABLE:
        cout << "DTR_CONTROL_DISABLE" << endl;
        break;
    case DTR_CONTROL_ENABLE:
        cout << "DTR_CONTROL_ENABLE" << endl;
        break;
    case DTR_CONTROL_HANDSHAKE:
        cout << "DTR_CONTROL_HANDSHAKE" << endl;
    }
}

bool openPort()
{
    // checking if port is closed
    if (CloseHandle(hCommDev))
    {
        cout << "Closing open port " << portName;
    }
    hCommDev = CreateFile(
        portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);
    cout << "Initial configuration:" << endl;
    printDcb(hCommDev, dcb);
    return hCommDev != INVALID_HANDLE_VALUE;
}

void modifyPortDcb(unsigned long BaudRate, int ByteSize, unsigned long fParity, int Parity, int StopBits)
{
    if (hCommDev != INVALID_HANDLE_VALUE)
    {
        dcb.DCBlength = sizeof(dcb);
        dcb.BaudRate = BaudRate;
        dcb.fParity = fParity;
        dcb.Parity = Parity;
        dcb.StopBits = StopBits;
        dcb.ByteSize = ByteSize;
        dcb.fDtrControl = DTR_CONTROL_DISABLE;
        SetCommState(hCommDev, &dcb);
    }
    printDcb(hCommDev, dcb);
}

void closePort()
{
    if (CloseHandle(hCommDev))
    {
        cout << "Closing open Port " << portName << endl;
    }
}

void setTimeouts(
    unsigned long ReadIntervalTimeout,
    unsigned long ReadTotalTimeoutMultiplier,
    unsigned long ReadTotalTimeoutConstant,
    unsigned long WriteTotalTimeoutMultiplier,
    unsigned long WriteTotalTimeoutConstant)
{

    if (GetCommTimeouts(hCommDev, &commTimeouts))
    {
        cout << "GetCommTimeouts works!" << endl;
        commTimeouts.ReadIntervalTimeout = ReadIntervalTimeout;
        commTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier;
        commTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant;
        commTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier;
        commTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant;
        if (!SetCommTimeouts(hCommDev, &commTimeouts))
        {
            cout << "setCommTimeouts Doesn't work!" << endl;
        }
    }
}

bool writeSerialPort(const char *buffer, DWORD numberOfBytesToWrite, DWORD *bytesWritten)
{
    if (WriteFile(hCommDev, buffer, numberOfBytesToWrite, bytesWritten, NULL) == 0)
    {
        cout << "Error with writing!" << endl;
        return false;
    }
    if (*bytesWritten < numberOfBytesToWrite)
    {
        cout << "Not all data was sent!" << endl;
        return false;
    }
    cout << "Write Serial port written:\n"
         << buffer << endl;
    return true;
}

bool readSerialPort(DWORD buff_size, ofstream *file)
{
    char *buffer = new char[buff_size];
    unsigned long numberOfBytesToRead = 0;
    if (ReadFile(hCommDev, buffer, buff_size, &numberOfBytesToRead, NULL))
    {
        if (numberOfBytesToRead >= 1)
        {
            file->write(buffer, buff_size);
            cout << "Data received:\n"
                 << buffer << endl;
            delete[] buffer;
            return true;
        }
    }
    delete[] buffer;
    cout << "Error with reading from serial" << endl;
    return false;
}

bool sendFileOverSerialPort(const string inputFilePath, const string outputFilePath)
{
    DWORD BUFF_SIZE = 8192 * 4;
    //    DWORD BUFF_SIZE = 16384;
    //    DWORD BUFF_SIZE = 32768;
    //    char buffer[BUFF_SIZE];
    char *buffer = new char[BUFF_SIZE];

    ifstream inputFile;
    ofstream outputFile;
    DWORD bytesWritten;
    long int totalBytesWritten = 0;

    inputFile.open(inputFilePath, ifstream::in | ifstream::binary);
    if (!inputFile.good())
    {
        std::cerr << "File " << inputFilePath << " does not exist!\n";
        return false;
    }
    outputFile.open(outputFilePath, ofstream::out | ofstream::binary);

    inputFile.seekg(0, inputFile.end);
    long int fileSize = inputFile.tellg();
    inputFile.seekg(0, inputFile.beg);

    while (inputFile.good() && !inputFile.eof())
    {
        fill(buffer, buffer + BUFF_SIZE, 0);
        cout << "Cleared buffer" << endl;

        inputFile.read(buffer, BUFF_SIZE);
        cout << "Read to buffer" << endl;
        streamsize bytesRead = inputFile.gcount();
        cout << "BytesRead: " << bytesRead << endl;

        if (bytesRead > 0)
        {
            if (!writeSerialPort(buffer, bytesRead, &bytesWritten))
            {
                return false;
            }
            cout << "written to serial" << endl;
            if (!readSerialPort(bytesWritten, &outputFile))
            {
                return false;
            }
            cout << "read from serial" << endl;
        }
        totalBytesWritten += bytesRead;
        cout << totalBytesWritten << endl;
        cout << fileSize << endl;
        cout << ((float)totalBytesWritten / fileSize) * 100 << "%" << endl;
    }
    if (totalBytesWritten != fileSize)
    {
        cout << "wrong bytes number " << totalBytesWritten << " vs " << fileSize << endl;
        return false;
    }
    inputFile.close();
    outputFile.close();
    cout << "File has been sent succesfully!" << endl;
    delete[] buffer;
    return true;
}

int main()
{
    int portNum;
    char *fullName = new char[5];
    std::cout << "Enter port number COM_\nex. input = 3 => opens COM3\n";
    std::cin >> portNum;
    sprintf(fullName, "COM%d", portNum);
    std::cout << fullName << "\n";
    portName = (LPCTSTR)fullName;
    if (openPort())
    {
        cout << "\nFinal configuration:" << endl;
        modifyPortDcb(CBR_256000, 8, true, ODDPARITY, ONESTOPBIT);
        setTimeouts(0, 5000, 0, 0, 0);

        string inputFilePath;
        string outputFilePath;
        std::cout << "Enter path to file that will be transfered: ";
        std::cin >> inputFilePath;
        std::cout << "\n";

        std::cout << "Enter path to where file will be transfered: ";
        std::cin >> outputFilePath;
        std::cout << "\n";

        sendFileOverSerialPort(inputFilePath, outputFilePath);
    }
    else
    {
        std::cerr << "Cannot open Port to transmission: " << fullName << ".\nCheck connection or try other port.\n";
    }

    closePort();
    return 0;
}
