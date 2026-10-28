#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "admin_auth.h"
#include "utils.h"
#include "common.h"

namespace fs = std::filesystem;

namespace {
fs::path originalDirectory;
fs::path testDirectory;
std::vector<std::string> inputValues;
std::size_t inputIndex = 0;

class AdminAuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        originalDirectory = fs::current_path();

        testDirectory = fs::temp_directory_path() /
            ("admin_auth_test_" +
             std::to_string(
                 static_cast<unsigned long long>(
                     std::rand()
                 )
             ) +
             "_" +
             std::to_string(
                 static_cast<unsigned long long>(
                     reinterpret_cast<std::uintptr_t>(this)
                 )
             ));

        fs::create_directories(testDirectory);
        fs::current_path(testDirectory);

        inputValues.clear();
        inputIndex = 0;
    }

    void TearDown() override {
        fs::current_path(originalDirectory);

        std::error_code ec;
        fs::remove_all(testDirectory, ec);
    }

    void writeAdminFile(const std::string& content) {
        std::ofstream file("admin.txt", std::ios::binary);
        ASSERT_TRUE(file.is_open());
        file << content;
        file.close();
        ASSERT_TRUE(file.good());
    }

    void provideInputs(
        std::initializer_list<std::string> values
    ) {
        inputValues.assign(values.begin(), values.end());
        inputIndex = 0;
    }
};
}  // namespace

// Test double: supplies predetermined values instead of reading stdin.
// Its signature must match utils.h exactly.
extern "C" void getStringInput(
    const char* message,
    char* buffer,
    int size
) {
    (void)message;

    if (size <= 0) {
        return;
    }

    if (inputIndex >= inputValues.size()) {
        buffer[0] = '\0';
        return;
    }

    const std::string& value = inputValues[inputIndex++];
    std::snprintf(buffer, static_cast<std::size_t>(size), "%s",
                  value.c_str());
}

TEST_F(AdminAuthTest, MissingAdminFileReturnsFailure) {
    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, InvalidStartMarkerReturnsFailure) {
    writeAdminFile(
        "INVALID_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, InvalidEndMarkerReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "INVALID_END\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, EmptyUsernameReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "\n"
        "secret\n"
        "ADMIN_END\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, EmptyPasswordReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "\n"
        "ADMIN_END\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, MissingCredentialLineReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, ExtraNonWhitespaceContentReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
        "unexpected\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}

TEST_F(AdminAuthTest, ExtraWhitespaceContentIsAccepted) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
        " \t\r\n"
    );

    provideInputs({"admin", "secret"});

    EXPECT_EQ(adminLogin(), SUCCESS);
}

TEST_F(AdminAuthTest, CorrectCredentialsReturnSuccess) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
    );

    provideInputs({"admin", "secret"});

    EXPECT_EQ(adminLogin(), SUCCESS);
    EXPECT_EQ(inputIndex, 2U);
}

TEST_F(AdminAuthTest, IncorrectUsernameThenCorrectCredentialsSucceed) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
    );

    provideInputs({
        "wrong-user", "secret",
        "admin", "secret"
    });

    EXPECT_EQ(adminLogin(), SUCCESS);
    EXPECT_EQ(inputIndex, 4U);
}

TEST_F(AdminAuthTest, IncorrectPasswordThenCorrectCredentialsSucceed) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
    );

    provideInputs({
        "admin", "wrong-password",
        "admin", "secret"
    });

    EXPECT_EQ(adminLogin(), SUCCESS);
    EXPECT_EQ(inputIndex, 4U);
}

TEST_F(AdminAuthTest, ThreeFailedAttemptsReturnFailure) {
    writeAdminFile(
        "ADMIN_START\n"
        "admin\n"
        "secret\n"
        "ADMIN_END\n"
    );

    provideInputs({
        "wrong1", "wrong1",
        "wrong2", "wrong2",
        "wrong3", "wrong3"
    });

    EXPECT_EQ(adminLogin(), FAILURE);
    EXPECT_EQ(inputIndex, 6U);
}

TEST_F(AdminAuthTest, WindowsLineEndingsAreAccepted) {
    writeAdminFile(
        "ADMIN_START\r\n"
        "admin\r\n"
        "secret\r\n"
        "ADMIN_END\r\n"
    );

    provideInputs({"admin", "secret"});

    EXPECT_EQ(adminLogin(), SUCCESS);
}

TEST_F(AdminAuthTest, OverlongCredentialLineReturnsFailure) {
    writeAdminFile(
        "ADMIN_START\n" +
        std::string(120, 'a') + "\n" +
        "secret\n"
        "ADMIN_END\n"
    );

    EXPECT_EQ(adminLogin(), FAILURE);
}