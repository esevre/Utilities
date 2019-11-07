//
// Created by Erik Sevre on 11/4/2019.
//
///  Code to handle header file for RIFF formatted WAV files
/// This should work to read in the general case, and provides
/// the ability to print the values to check what is going on
/// as well. I'm not sure about the all the different wav formats
/// so some of the data is just read as a block for the "extension"
/// type of WAV file.
///


#pragma once

#include <array>
#include <fstream>
#include <iostream>


/// Extension class to handle reading non-PCM WAV files.
/// PCM files will call empty functions in Extension.
struct Extension
{
    /// Use enum class to manage different types of extensions
    enum class RiffType { PCM, NonPCM, Extensible, Undefined};

    /// Data shortcuts and internal constants
    using QuadChar = std::array<char, 4>;
    using QuadSInt = int;
    using HalfSInt = short;
    using SubChunk = std::array<char, 16>;
    static constexpr size_t FormatSize = 16;
    static constexpr size_t QuadSize = 4;
    static constexpr size_t HalfSize = 2;

    /// Default constructor
    Extension()=default;
    /// Construct and set type
    explicit Extension(RiffType type) {
        Type = type;
    }

    /// Only Extensible and NonPCM types will have extra data to read
    void read(std::fstream &infile) {
        if (Type == RiffType::Extensible) {
            read_extension(infile);
        } else if (Type == RiffType::NonPCM){
            read_non_pcm(infile);
        }
    }
    /// Only Extensible and NonPCM types will have extra data to print
    void print() {
        if (Type == RiffType::Extensible) {
            print_extension();
        } else if (Type == RiffType::NonPCM){
            print_non_pcm();
        }
    }

private:
    /// Methods for internal use only
    /// print & read helpers for different Extended types
    /// PCM files don't have any extra data so no function is called
    void print_non_pcm() {
        std::cout << "Extension Size : " << ExtensionSize << '\n';
        std::cout << "ChunkID : " << str(ChunkID) << '\n';
        std::cout << "ChunkSize : " << ChunkSize << '\n';
        std::cout << "SampleLength : " << SampleLength << '\n';

    }
    void print_extension() {
        std::cout << "Extension Size : " << ExtensionSize << '\n';
        std::cout << "ValidBitsPerSample : " << ValidBitsPerSample << '\n';
        std::cout << "ChannelMask : " << ChannelMask << '\n';
        std::cout << "Subformat (size) : " << sizeof(SubFormat) << '\n';
        std::cout << "ChunkID : " << str(ChunkID) << '\n';
        std::cout << "ChunkSize : " << ChunkSize << '\n';
        std::cout << "SampleLength : " << SampleLength << '\n';
    }
    void read_non_pcm(std::fstream &infile){
        infile.read((char*)&ExtensionSize, HalfSize);
        infile.read((char*)&ChunkID, QuadSize);
        infile.read((char*)&ChunkSize, QuadSize);
        infile.read((char*)&SampleLength, QuadSize);
    }
    void read_extension(std::fstream &infile) {
        infile.read((char*)&ExtensionSize, HalfSize);
        infile.read((char*)&ValidBitsPerSample, HalfSize);
        infile.read((char*)&ChannelMask, QuadSize);
        infile.read((char*)&SubFormat, FormatSize);
        infile.read((char*)&ChunkID, QuadSize);
        infile.read((char*)&ChunkSize, QuadSize);
        infile.read((char*)&SampleLength, QuadSize);
    }
    std::string str(const QuadChar &quadChar) {
        std::string s = "\"";
        for (const auto &c : quadChar) {
            s += c;
        }
        s += '\"';
        return s;
    }

public:
    /// All data is default initialized to zero
    /// arrays are known size, so filled manually by default
    RiffType Type = RiffType::Undefined;
    HalfSInt ExtensionSize=0;
    HalfSInt ValidBitsPerSample=0;
    QuadSInt ChannelMask=0;
    // todo: figure out how the subformat chunk works to extract more useful information
    SubChunk SubFormat={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    QuadChar ChunkID={0,0,0,0};
    QuadSInt ChunkSize=0;
    QuadSInt SampleLength=0;
};

class WavHeader {
public:
    /// Data shortcuts
    using QuadChar = std::array<char, 4>;
    using QuadSInt = int;
    using HalfSInt = short;
    static constexpr size_t QuadSize = 4;
    static constexpr size_t HalfSize = 2;

private:
    /// Custom internal helper functions
    std::string str(const QuadChar &quadChar) {
        std::string s = "\"";
        for (const auto &c : quadChar) {
            s += c;
        }
        s += '\"';
        return s;
    }

public:
    /// Reads wav header from std::fstream
    /// Manually step through and read each variable
    void read(std::fstream &infile) {
        infile.read((char*)&ChunkID, QuadSize);
        infile.read((char*)&ChunkSize, QuadSize);
        infile.read((char*)&Format, QuadSize);
        infile.read((char*)&SubChunk1_ID, QuadSize);
        infile.read((char*)&SubChunk1_size, QuadSize);
        infile.read((char*)&AudioFormat, HalfSize);
        infile.read((char*)&NumChannels, HalfSize);
        infile.read((char*)&SampleRate, QuadSize);
        infile.read((char*)&ByteRate, QuadSize);
        infile.read((char*)&BlockAlign, HalfSize);
        infile.read((char*)&BitsPerSample, HalfSize);

        /// Check to see if extra information is in the header
        /// If it is there, then read it
        /// ExtendedData calls the correct read method based on header type
        /// Regular (PCM) headers will call an empty function.
        setup_FormatType();
        ExtendedData.read(infile);

        infile.read((char*)&SubChunk2_ID, QuadSize);
        infile.read((char*)&SubChunk2_size, QuadSize);
    }

    /// Extended data should be one of 3 types
    /// Bad data, or future changes, may lead to undefined format types
    /// PCM : Is a "simple" WAV file using old standards
    /// Non-PCM : For nonPCM formatted chunks
    /// Extensible : Indicates that there is an extension to the format chunk
    void setup_FormatType() {
        if (SubChunk1_size == 16) {
            ExtendedData = Extension(Extension::RiffType::PCM);
        } else if (SubChunk1_size == 18) {
            ExtendedData = Extension(Extension::RiffType::NonPCM);
        } else if (SubChunk1_size == 40) {
            ExtendedData = Extension(Extension::RiffType::Extensible);
        } else {
            ExtendedData = Extension(Extension::RiffType::Undefined);
        }
    }

    /// Function to print and test that data is reading correctly
    void print() {
        std::cout << "ChunkID : " << str(ChunkID) << '\n';
        std::cout << "ChunkSize : " << ChunkSize << '\n';
        std::cout << "Format : " << str(Format) << '\n';
        std::cout << "SubChunk1_ID : " << str(SubChunk1_ID) << '\n';
        std::cout << "SubChunk1_size : " << SubChunk1_size << '\n';
        std::cout << "AudioFormat : " << AudioFormat << '\n';
        std::cout << "NumChannels : " << NumChannels << '\n';
        std::cout << "SampleRate : " << SampleRate << '\n';
        std::cout << "ByteRate : " << ByteRate << '\n';
        std::cout << "BlockAlign : " << BlockAlign << '\n';
        std::cout << "BitsPerSample : " << BitsPerSample << '\n';
        ExtendedData.print();
        std::cout << "SubChunk2_ID : " << str(SubChunk2_ID) << '\n';
        std::cout << "SubChunk2_size : " << SubChunk2_size << '\n';
    }

public:
    /// Header data information
    QuadChar ChunkID={0,0,0,0};
    QuadSInt ChunkSize=0;
    QuadChar Format={0,0,0,0};
    QuadChar SubChunk1_ID={0,0,0,0};
    QuadSInt SubChunk1_size=0;
    HalfSInt AudioFormat=0;
    HalfSInt NumChannels=0;
    QuadSInt SampleRate=0;
    QuadSInt ByteRate=0;
    HalfSInt BlockAlign=0;
    HalfSInt BitsPerSample=0;
    Extension ExtendedData;
    QuadChar SubChunk2_ID={0,0,0,0};
    QuadSInt SubChunk2_size=0;
};


