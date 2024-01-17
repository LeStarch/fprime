#include "Os/test/ut/file/CommonTests.hpp"
#include <gtest/gtest.h>
#include "Os/File.hpp"

namespace Os {
namespace Test {
namespace File {

void Tester::assert_file_opened(std::string& path, Os::File::Mode newly_opened_mode, bool overwrite) {
    // Assert the that the file is opened in some mode
    ASSERT_NE(this->file.mode, Os::File::Mode::OPEN_NO_MODE) << "File is in unexpected mode";
    ASSERT_TRUE(this->file.isOpen()) << "`isOpen()` failed to indicate file is open";
    this->current_path = path;

    // When the open mode has been specified assert that is in an exact state
    if (Os::File::Mode::OPEN_NO_MODE != newly_opened_mode) {

        ASSERT_EQ(this->file.mode, newly_opened_mode) << "File is in unexpected mode";
        const bool truncate = (Os::File::Mode::OPEN_CREATE == newly_opened_mode) && overwrite;

        // The case where the file should not already exist
        if (this->filesystem.count(this->current_path) == 0) {
            this->filesystem[this->current_path] = std::unique_ptr<FileData>(new FileData);
        }

        // Grab the data to work with and truncate if necessary
        FileData& data = *this->filesystem.at(this->current_path).get();
        if (truncate) {
            data.data.clear();
        }
        ASSERT_EQ(this->size(this->current_path), data.data.size());
        data.pointer = (newly_opened_mode != Os::File::Mode::OPEN_APPEND) ? 0 : data.data.size();
    }
    ASSERT_GE(this->filesystem.at(this->current_path).get()->pointer, 0);
    ASSERT_EQ(this->position(), this->filesystem.at(this->current_path).get()->pointer);
}

void Tester::assert_file_read(const unsigned char* read_data, FwSizeType size_read, FwSizeType size_desired) {
    FileData& data = *this->filesystem.at(this->current_path).get();
    ASSERT_GE(data.data.size(), data.pointer);

    FwSizeType remaining = static_cast<FwSizeType>(data.data.size() - data.pointer);
    FwSizeType expected_read = FW_MIN(size_desired, remaining);
    const FwSizeType new_pointer = data.pointer + expected_read;
    ASSERT_EQ(size_read, expected_read);

    // Check expected read bytes
    for (FwSizeType i = 0; i < size_desired; i++, data.pointer++) {
        // End of file
        if (data.pointer == static_cast<FwSizeType>(data.data.size())) {
            ASSERT_EQ(i, expected_read);
            break;
        }
        ASSERT_EQ(read_data[i], data.data.at(data.pointer));
    }
    ASSERT_EQ(data.pointer, new_pointer);
    ASSERT_EQ(this->position(), data.pointer);
}

void Tester::assert_file_write(const unsigned char* write_data, FwSizeType size_written, FwSizeType size_desired) {
    FileData& data = *this->filesystem.at(this->current_path).get();
    ASSERT_EQ(size_written, size_desired); // Baring errors
    const FwSizeType new_pointer = size_desired + data.pointer;

    // Assure that the file size matches
    for (FwSizeType i = 0; i < size_desired; i++, data.pointer++) {
        if (data.pointer < static_cast<FwSizeType>(data.data.size())) {
            data.data[data.pointer] = write_data[i];
        } else {
            data.data.push_back(write_data[i]);
        }
    }
    ASSERT_EQ(data.pointer, new_pointer);
    ASSERT_EQ(static_cast<FwSizeType>(data.data.size()), this->size(this->current_path));
    ASSERT_EQ(this->position(), data.pointer);
}

void Tester::assert_file_seek(FwSizeType seek_desired, bool absolute) {
    FileData& data = *this->filesystem.at(this->current_path).get();
    ASSERT_TRUE(seek_desired > 0 || not absolute);
    data.pointer = (absolute) ? seek_desired : (data.pointer + seek_desired);
    ASSERT_EQ(this->position(), data.pointer);
}

void Tester::assert_valid_mode_status(Os::File::Status &status) const {
    if (Os::File::Mode::OPEN_NO_MODE == this->mode) {
        ASSERT_EQ(status, Os::File::Status::NOT_OPENED);
    } else {
        ASSERT_EQ(status, Os::File::Status::INVALID_MODE);
    }
}

void Tester::assert_file_closed() {
    ASSERT_EQ(this->file.mode, Os::File::Mode::OPEN_NO_MODE) << "File is in unexpected mode";
    ASSERT_FALSE(this->file.isOpen()) << "`isOpen()` failed to indicate file is open";
    this->current_path.clear();
}
}  // File
}  // Test
}  // Os

//! Category of tests to check for invalid argument assertions
class InvalidArguments : public Functionality {};

//! Category of tests to check for functional i/o operations
class FunctionalIO : public Functionality {
    //! Setup function delegating to UT setup function
    virtual void SetUp() override { Os::Test::File::setUp(true); }
};

// Ensure that open mode changes work reliably
TEST_F(Functionality, OpenWithCreation) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();

    Os::Test::File::Tester::OpenFileCreate rule(false);
    rule.apply(*tester);
}

// Ensure that close mode changes work reliably
TEST_F(Functionality, Close) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();

    Os::Test::File::Tester::OpenFileCreate create_rule(false);
    Os::Test::File::Tester::CloseFile close_rule;
    create_rule.apply(*tester);
    close_rule.apply(*tester);
}

// Ensure that open mode changes work reliably
TEST_F(Functionality, OpenInvalidModes) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();

    Os::Test::File::Tester::OpenFileCreate original_open(false);
    Os::Test::File::Tester::OpenInvalidModes invalid_open;
    original_open.apply(*tester);
    invalid_open.apply(*tester);
}

// Ensure that Os::File properly refuses preallocate calls when not open
TEST_F(Functionality, PreallocateWithoutOpen) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::PreallocateWithoutOpen rule;
    rule.apply(*tester);
}

// Ensure that Os::File properly refuses seek calls when not open
TEST_F(Functionality, SeekWithoutOpen) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::SeekWithoutOpen rule;
    rule.apply(*tester);
}

// Ensure that Os::File properly refuses flush calls when not open and when reading
TEST_F(Functionality, FlushInvalidModes) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::FlushInvalidModes flush_rule;
    Os::Test::File::Tester::OpenFileCreate open_rule(false);
    Os::Test::File::Tester::CloseFile close_rule;
    Os::Test::File::Tester::OpenForRead open_read;

    // Test flush in closed state
    flush_rule.apply(*tester);

    // Used to create the test file an open in read-mode correctly
    open_rule.apply(*tester);
    close_rule.apply(*tester);
    tester->assert_file_closed();
    open_read.apply(*tester);
    tester->assert_file_opened(tester->current_path);

    // Check that a read-mode file cannot flush
    flush_rule.apply(*tester);
}

// Ensure that Os::File properly refuses read calls when not open and when reading
TEST_F(Functionality, ReadInvalidModes) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenFileCreate open_rule(false);
    Os::Test::File::Tester::CloseFile close_rule;
    Os::Test::File::Tester::OpenForWrite open_write;
    Os::Test::File::Tester::ReadInvalidModes read_rule;

    // Test read in closed state
    read_rule.apply(*tester);
    tester->assert_file_closed();

    // Used to create the test file and ensure reads cannot happen in read-mode
    open_rule.apply(*tester);
    read_rule.apply(*tester);
    close_rule.apply(*tester);
    tester->assert_file_closed();

    // Used to open (now existent) file in write-mode
    open_write.apply(*tester);
    tester->assert_file_opened(tester->current_path);

    // Check that a read won't work on write-mode data
    read_rule.apply(*tester);
}

// Ensure that Os::File properly refuses write calls when not open and when reading
TEST_F(Functionality, WriteInvalidModes) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenFileCreate open_rule(false);
    Os::Test::File::Tester::CloseFile close_rule;
    Os::Test::File::Tester::OpenForRead open_read;
    Os::Test::File::Tester::WriteInvalidModes write_rule;

    // Test write in closed state
    write_rule.apply(*tester);
    tester->assert_file_closed();

    // Used to create the test file in read-mode correctly
    open_rule.apply(*tester);
    close_rule.apply(*tester);
    tester->assert_file_closed();
    open_read.apply(*tester);
    tester->assert_file_opened(tester->current_path);

    // Check that a write won't work on write-mode data
    write_rule.apply(*tester);
}

// Ensure a write followed by a read produces valid data
TEST_F(FunctionalIO, WriteReadBack) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenFileCreate open_rule(false);
    Os::Test::File::Tester::Write write_rule;
    Os::Test::File::Tester::CloseFile close_rule;
    Os::Test::File::Tester::OpenForRead open_read;
    Os::Test::File::Tester::Read read_rule;

    open_rule.apply(*tester);
    write_rule.apply(*tester);
    close_rule.apply(*tester);
    open_read.apply(*tester);
    read_rule.apply(*tester);
}

// Ensure a write followed by a read produces valid data
TEST_F(FunctionalIO, WriteReadSeek) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenFileCreate open_rule(false);
    Os::Test::File::Tester::Write write_rule;
    Os::Test::File::Tester::CloseFile close_rule;
    Os::Test::File::Tester::OpenForRead open_read;
    Os::Test::File::Tester::Read read_rule;
    Os::Test::File::Tester::Seek seek_rule;

    open_rule.apply(*tester);
    write_rule.apply(*tester);
    close_rule.apply(*tester);
    open_read.apply(*tester);
    read_rule.apply(*tester);
    seek_rule.apply(*tester);
}


// Ensure open prevents nullptr as path
TEST_F(InvalidArguments, OpenBadPath) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenIllegalPath rule;
    rule.apply(*tester);
}

// Ensure open prevents bad modes
TEST_F(InvalidArguments, OpenBadMode) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::OpenIllegalMode rule;
    rule.apply(*tester);
}

// Ensure preallocate prevents bad offset
TEST_F(InvalidArguments, PreallocateBadOffset) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::PreallocateIllegalOffset rule;
    rule.apply(*tester);
}

// Ensure preallocate prevents bad length
TEST_F(InvalidArguments, PreallocateBadLength) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::PreallocateIllegalLength rule;
    rule.apply(*tester);
}

// Ensure preallocate prevents bad length
TEST_F(InvalidArguments, SeekAbsoluteWithNegativeLength) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::SeekIllegal rule;
    rule.apply(*tester);
}

// Ensure read prevents bad buffer pointers
TEST_F(InvalidArguments, ReadInvalidBuffer) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::ReadIllegalBuffer rule;
    rule.apply(*tester);
}

// Ensure read prevents bad sizes
TEST_F(InvalidArguments, ReadInvalidSize) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::ReadIllegalSize rule;
    rule.apply(*tester);
}

// Ensure write prevents bad buffer pointers
TEST_F(InvalidArguments, WriteInvalidBuffer) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::WriteIllegalBuffer rule;
    rule.apply(*tester);
}

// Ensure write prevents bad sizes
TEST_F(InvalidArguments, WriteInvalidSize) {
    std::unique_ptr<Os::Test::File::Tester> tester = Os::Test::File::get_tester_implementation();
    Os::Test::File::Tester::WriteIllegalSize rule;
    rule.apply(*tester);
}