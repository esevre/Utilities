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

    /// Default constructor should never be called
    Extension()=delete;
    /// Construct and set type
    explicit Extension(RiffType type) {
        riffType = type;
    }

    /// Only Extensible and NonPCM types will have extra data to read
    void read(std::fstream &infile) {
        if (riffType == RiffType::Extensible) {
            read_extension(infile);
        } else if (riffType == RiffType::NonPCM){
            read_non_pcm(infile);
        }
    }
    /// Only Extensible and NonPCM types will have extra data to print
    void print() const {
        if (riffType == RiffType::Extensible) {
            print_extension();
        } else if (riffType == RiffType::NonPCM){
            print_non_pcm();
        }
    }

private:
    /// Methods for internal use only
    /// print & read helpers for different Extended types
    /// PCM files don't have any extra data so no function is called
    void print_non_pcm() const {
        std::cout << "Extension Size : " << extensionSize << '\n';
        std::cout << "chunkID : " << quad_char_string(chunkID) << '\n';
        std::cout << "ChunkSize : " << chunkSize << '\n';
        std::cout << "SampleLength : " << sampleLength << '\n';

    }
    void print_extension() const {
        std::cout << "Extension Size : " << extensionSize << '\n';
        std::cout << "validBitsPerSample : " << validBitsPerSample << '\n';
        std::cout << "channelMask : " << channelMask << '\n';
        std::cout << "Subformat (size) : " << sizeof(subFormat) << '\n';
        std::cout << "chunkID : " << quad_char_string(chunkID) << '\n';
        std::cout << "ChunkSize : " << chunkSize << '\n';
        std::cout << "SampleLength : " << sampleLength << '\n';
    }
    void read_non_pcm(std::fstream &infile){
        infile.read((char*)&extensionSize, HalfSize);
        infile.read((char*)&chunkID, QuadSize);
        infile.read((char*)&chunkSize, QuadSize);
        infile.read((char*)&sampleLength, QuadSize);
    }
    void read_extension(std::fstream &infile) {
        infile.read((char*)&extensionSize, HalfSize);
        infile.read((char*)&validBitsPerSample, HalfSize);
        infile.read((char*)&channelMask, QuadSize);
        infile.read((char*)&subFormat, FormatSize);
        infile.read((char*)&chunkID, QuadSize);
        infile.read((char*)&chunkSize, QuadSize);
        infile.read((char*)&sampleLength, QuadSize);
    }

    [[nodiscard]] std::string quad_char_string(const QuadChar &quadChar) const {
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
    RiffType riffType = RiffType::Undefined;
    HalfSInt extensionSize=0;
    HalfSInt validBitsPerSample=0;
    QuadSInt channelMask=0;
    // todo: figure out how the subformat chunk works to extract more useful information
    SubChunk subFormat={0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    QuadChar chunkID={0, 0, 0, 0};
    QuadSInt chunkSize=0;
    QuadSInt sampleLength=0;
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
    [[nodiscard]] std::string quad_char_string(const QuadChar &quadChar) const {
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
        infile.read((char*)&chunkId, QuadSize);
        infile.read((char*)&chunkSize, QuadSize);
        infile.read((char*)&format, QuadSize);
        infile.read((char*)&subChunk1Id, QuadSize);
        infile.read((char*)&subChunk1Size, QuadSize);
        infile.read((char*)&audioFormat, HalfSize);
        infile.read((char*)&numChannels, HalfSize);
        infile.read((char*)&sampleRate, QuadSize);
        infile.read((char*)&byteRate, QuadSize);
        infile.read((char*)&blockAlign, HalfSize);
        infile.read((char*)&bitsPerSample, HalfSize);

        /// Check to see if extra information is in the header
        /// If it is there, then read it
        /// ExtendedData calls the correct read method based on header type
        /// Regular (PCM) headers will call an empty function.
        set_format_type();
        extendedData.read(infile);

        infile.read((char*)&subChunk2Id, QuadSize);
        infile.read((char*)&subChunk2Size, QuadSize);
    }

    /// Extended data should be one of 3 types
    /// Bad data, or future changes, may lead to undefined format types
    /// PCM : Is a "simple" WAV file using old standards
    /// Non-PCM : For nonPCM formatted chunks
    /// Extensible : Indicates that there is an extension to the format chunk
    void set_format_type() {
        if (subChunk1Size == 16) {
            extendedData = Extension(Extension::RiffType::PCM);
        } else if (subChunk1Size == 18) {
            extendedData = Extension(Extension::RiffType::NonPCM);
        } else if (subChunk1Size == 40) {
            extendedData = Extension(Extension::RiffType::Extensible);
        } else {
            extendedData = Extension(Extension::RiffType::Undefined);
        }
    }

    /// Function to print and test that data is reading correctly
    void print() const {
        std::cout << "ChunkID : " << quad_char_string(chunkId) << '\n';
        std::cout << "ChunkSize : " << chunkSize << '\n';
        std::cout << "Format : " << quad_char_string(format) << '\n';
        std::cout << "SubChunk1_ID : " << quad_char_string(subChunk1Id) << '\n';
        std::cout << "SubChunk1_size : " << subChunk1Size << '\n';
        std::cout << "AudioFormat : " << audioFormat << '\n';
        std::cout << "NumChannels : " << numChannels << '\n';
        std::cout << "SampleRate : " << sampleRate << '\n';
        std::cout << "ByteRate : " << byteRate << '\n';
        std::cout << "BlockAlign : " << blockAlign << '\n';
        std::cout << "BitsPerSample : " << bitsPerSample << '\n';
        extendedData.print();
        std::cout << "SubChunk2_ID : " << quad_char_string(subChunk2Id) << '\n';
        std::cout << "SubChunk2_size : " << subChunk2Size << '\n';
    }

public:
    /// Header data information
    QuadChar chunkId={0, 0, 0, 0};
    QuadSInt chunkSize=0;
    QuadChar format={0, 0, 0, 0};
    QuadChar subChunk1Id={0, 0, 0, 0};
    QuadSInt subChunk1Size=0;
    HalfSInt audioFormat=0;
    HalfSInt numChannels=0;
    QuadSInt sampleRate=0;
    QuadSInt byteRate=0;
    HalfSInt blockAlign=0;
    HalfSInt bitsPerSample=0;
    Extension extendedData;
    QuadChar subChunk2Id={0, 0, 0, 0};
    QuadSInt subChunk2Size=0;
};


