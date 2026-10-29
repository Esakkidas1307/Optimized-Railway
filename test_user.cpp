#include <gtest/gtest.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <sys/stat.h>
extern "C" {
#include "user.h"
#include "passenger.h"
#include "utils.h"
#include "date_time.h"
#include "optimization_runtime.h"
}

/*
 * User module tests.
 * Tests use temporary stdin files to provide console input.
 */

class UserModuleTest : public ::testing::Test {
protected:
    const char *inputFile = "test_user_stdin.txt";

    void SetUp() override {
        clearUsers();
        std::remove("users.txt");
        std::remove("users.txt.tmp");
        std::remove("userlog.txt");
    }

    void TearDown() override {
        clearUsers();
        std::remove("users.txt");
        std::remove("users.txt.tmp");
        std::remove("userlog.txt");
        std::remove(inputFile);
        clearerr(stdin);
    }

    void clearUsers() {
        while (userHead != nullptr) {
            User *next = userHead->next;
            std::free(userHead);
            userHead = next;
        }
        userHead = nullptr;
    }

    void feedStdin(const std::string &input) {
        FILE *file = std::fopen(inputFile, "w");
        ASSERT_NE(file, nullptr);
        std::fputs(input.c_str(), file);
        std::fclose(file);

        ASSERT_NE(std::freopen(inputFile, "r", stdin), nullptr);
    }

    void addUser(int id, const char *username, const char *password) {
        User *u = static_cast<User *>(std::calloc(1, sizeof(User)));
        ASSERT_NE(u, nullptr);

        u->passengerId = id;
        std::snprintf(u->username, sizeof(u->username), "%s", username);
        std::snprintf(u->password, sizeof(u->password), "%s", password);

        u->next = userHead;
        if (userHead != nullptr) {
            userHead->prev = u;
        }
        userHead = u;
    }

    int countUsers() {
        int count = 0;
        for (User *u = userHead; u != nullptr; u = u->next) {
            ++count;
        }
        return count;
    }

    bool fileExists(const char *filename) {
        std::ifstream file(filename);
        return file.good();
    }
};

TEST_F(UserModuleTest, InitializeCreatesUserFile) {
    initializeUserModule();

    EXPECT_TRUE(fileExists("users.txt"));
}

TEST_F(UserModuleTest, RegisterUserCreatesNewUser) {
    feedStdin("alice\npass123\n");

    testing::internal::CaptureStdout();
    int id = registerUser();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_GT(id, 100);
    ASSERT_NE(userHead, nullptr);
    EXPECT_STREQ(userHead->username, "alice");
    EXPECT_STREQ(userHead->password, "pass123");
    EXPECT_NE(output.find("Registration Successful"), std::string::npos);
}

TEST_F(UserModuleTest, RegisterUserGeneratesSequentialIds) {
    addUser(105, "existing", "pw");
    feedStdin("newuser\nnewpass\n");

    int id = registerUser();

    EXPECT_EQ(id, 106);
    ASSERT_NE(userHead, nullptr);
    EXPECT_STREQ(userHead->username, "newuser");
}

TEST_F(UserModuleTest, RegisterUserRejectsDuplicateUsername) {
    addUser(101, "alice", "oldpass");
    feedStdin("alice\nalice2\nnewpass\n");

    testing::internal::CaptureStdout();
    int id = registerUser();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_GT(id, 0);
    EXPECT_NE(output.find("Username Already Exists"), std::string::npos);
    ASSERT_NE(userHead, nullptr);
    EXPECT_STREQ(userHead->username, "alice2");
}

TEST_F(UserModuleTest, LoginSucceedsWithValidCredentials) {
    addUser(123, "alice", "pass123");
    feedStdin("alice\npass123\n");

    testing::internal::CaptureStdout();
    int id = loginUser();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(id, 123);
    EXPECT_NE(output.find("Login Successful"), std::string::npos);
}

TEST_F(UserModuleTest, LoginRejectsInvalidPassword) {
    addUser(123, "alice", "pass123");
    feedStdin("alice\nwrong\nalice\nwrong\nalice\nwrong\n");

    testing::internal::CaptureStdout();
    int result = loginUser();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(result, FAILURE);
    EXPECT_NE(output.find("Maximum login attempts reached"),
              std::string::npos);
}

TEST_F(UserModuleTest, LoginRejectsUnknownUsername) {
    addUser(123, "alice", "pass123");
    feedStdin("unknown\npw\nunknown\npw\nunknown\npw\n");

    testing::internal::CaptureStdout();
    int result = loginUser();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(result, FAILURE);
    EXPECT_NE(output.find("Invalid username or password"),
              std::string::npos);
}

TEST_F(UserModuleTest, SaveUserDataWritesUserRecord) {
    addUser(123, "alice", "pass123");

    EXPECT_EQ(saveUserData(), SUCCESS);
    EXPECT_TRUE(fileExists("users.txt"));

    std::ifstream file("users.txt");
    std::string contents((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    EXPECT_NE(contents.find("USER_START"), std::string::npos);
    EXPECT_NE(contents.find("123"), std::string::npos);
    EXPECT_NE(contents.find("alice"), std::string::npos);
    EXPECT_NE(contents.find("pass123"), std::string::npos);
    EXPECT_NE(contents.find("USER_END"), std::string::npos);
}

TEST_F(UserModuleTest, LoadUserDataRestoresSavedUser) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n"
             << "123\n"
             << "alice\n"
             << "pass123\n"
             << "USER_END\n";
    }

    loadUserData();

    ASSERT_NE(userHead, nullptr);
    EXPECT_EQ(userHead->passengerId, 123);
    EXPECT_STREQ(userHead->username, "alice");
    EXPECT_STREQ(userHead->password, "pass123");
}

TEST_F(UserModuleTest, LoadUserDataSkipsDuplicateRecords) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n101\nalice\npw1\nUSER_END\n"
             << "USER_START\n102\nalice\npw2\nUSER_END\n"
             << "USER_START\n101\nbob\npw3\nUSER_END\n";
    }

    testing::internal::CaptureStdout();
    loadUserData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(countUsers(), 1);
    EXPECT_NE(output.find("Invalid or duplicate user record skipped"),
              std::string::npos);
}

TEST_F(UserModuleTest, LoadUserDataSkipsInvalidPassengerId) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n0\nalice\npw\nUSER_END\n";
    }

    loadUserData();

    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataSkipsEmptyUsername) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n101\n\npw\nUSER_END\n";
    }

    loadUserData();

    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataSkipsEmptyPassword) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n101\nalice\n\nUSER_END\n";
    }

    loadUserData();

    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataSkipsMalformedRecord) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n101\nalice\npw\nINVALID_END\n";
    }

    testing::internal::CaptureStdout();
    loadUserData();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_EQ(userHead, nullptr);
    EXPECT_NE(output.find("Invalid user record skipped"),
              std::string::npos);
}

TEST_F(UserModuleTest, LoadUserDataHandlesMissingFile) {
    std::remove("users.txt");

    EXPECT_NO_THROW(loadUserData());
    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, SaveUserDataHandlesMultipleUsers) {
    addUser(101, "alice", "pw1");
    addUser(102, "bob", "pw2");

    EXPECT_EQ(saveUserData(), SUCCESS);

    clearUsers();
    loadUserData();

    EXPECT_EQ(countUsers(), 2);
}

TEST_F(UserModuleTest, RegistrationPersistsUserData) {
    feedStdin("alice\npass123\n");

    int id = registerUser();
    ASSERT_GT(id, 0);

    clearUsers();
    loadUserData();

    ASSERT_NE(userHead, nullptr);
    EXPECT_EQ(userHead->passengerId, id);
    EXPECT_STREQ(userHead->username, "alice");
    EXPECT_STREQ(userHead->password, "pass123");
}

TEST_F(UserModuleTest, InitializeHandlesUserFileCreationFailure) {
    // A directory named users.txt makes fopen("users.txt", "a") fail.
    ASSERT_EQ(mkdir("users.txt", 0700), 0);

    testing::internal::CaptureStdout();
    initializeUserModule();
    std::string output = testing::internal::GetCapturedStdout();

    EXPECT_NE(output.find("Unable To Create User File"), std::string::npos);

    ASSERT_EQ(rmdir("users.txt"), 0);
}

TEST_F(UserModuleTest, SaveUserDataHandlesTemporaryPathFailure) {
    // A directory named users.txt.tmp prevents fopen(..., "w").
    ASSERT_EQ(mkdir("users.txt.tmp", 0700), 0);

    EXPECT_EQ(saveUserData(), FAILURE);

    ASSERT_EQ(rmdir("users.txt.tmp"), 0);
}

TEST_F(UserModuleTest, SaveUserDataHandlesRenameFailure) {
    addUser(123, "alice", "pass123");

    // Force rename(temp, destination) to fail by making destination a directory.
    ASSERT_EQ(mkdir("users.txt", 0700), 0);

    EXPECT_EQ(saveUserData(), FAILURE);

    // saveUserData should remove the temporary file on rename failure.
    EXPECT_FALSE(fileExists("users.txt.tmp"));

    ASSERT_EQ(rmdir("users.txt"), 0);
}

TEST_F(UserModuleTest, SaveUserDataHandlesFlushFailure) {
    addUser(123, "alice", "pass123");

    // On Linux, /dev/full fails writes with ENOSPC.
    // Use a symlink at the temporary filename to exercise the flush error.
    ASSERT_EQ(symlink("/dev/full", "users.txt.tmp"), 0);

    EXPECT_EQ(saveUserData(), FAILURE);

    // saveUserData removes the temporary symlink on the error path.
    EXPECT_FALSE(fileExists("users.txt.tmp"));
}

TEST_F(UserModuleTest, LoadUserDataRejectsOverlongUsername) {
    std::string longUsername(150, 'u');

    {
        std::ofstream file("users.txt");
        file << "USER_START\n"
             << "123\n"
             << longUsername << "\n"
             << "pass123\n"
             << "USER_END\n";
    }

    loadUserData();

    // The overlong username is cleared and the record is rejected.
    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataRejectsOverlongPassword) {
    std::string longPassword(150, 'p');

    {
        std::ofstream file("users.txt");
        file << "USER_START\n"
             << "123\n"
             << "alice\n"
             << longPassword << "\n"
             << "USER_END\n";
    }

    loadUserData();

    // The overlong password is cleared and the record is rejected.
    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataHandlesIncompleteUsernameRecord) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n123\n";
    }

    EXPECT_NO_THROW(loadUserData());
    EXPECT_EQ(userHead, nullptr);
}

TEST_F(UserModuleTest, LoadUserDataHandlesIncompletePasswordRecord) {
    {
        std::ofstream file("users.txt");
        file << "USER_START\n123\nalice\n";
    }

    EXPECT_NO_THROW(loadUserData());
    EXPECT_EQ(userHead, nullptr);
}
