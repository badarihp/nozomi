#include <gtest/gtest.h>

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>

#include "src/StreamingFileHandler.h"
#include "test/Common.h"

namespace fs = boost::filesystem;
using namespace std;
using namespace proxygen;
using boost::filesystem::path;
using folly::IOBuf;

namespace nozomi {
namespace test {

struct TempDir {
  path tempDir;
  TempDir() {
    tempDir = fs::temp_directory_path() / fs::unique_path() / fs::unique_path();
    fs::create_directories(tempDir);
    LOG(INFO) << "Created temp directory " << tempDir;
  }
  virtual ~TempDir() {
    LOG(INFO) << "Removing " << tempDir << endl;
    tempDir.remove_filename();
    LOG(INFO) << "Removing " << tempDir << endl;
    fs::remove_all(tempDir);
  }
};

struct StreamingFileHandlerTest : ::testing::Test {
  TempDir tempDir;
  std::unique_ptr<proxygen::HTTPMessage> requestMessage;
  std::unique_ptr<IOBuf> body;
  folly::EventBase evb;
  StreamingFileHandler httpHandler;
  TestResponseHandler responseHandler;

  StreamingFileHandlerTest()
      : httpHandler(tempDir.tempDir, 10, &evb, &evb),
        responseHandler(&httpHandler) {
    requestMessage = std::make_unique<HTTPMessage>();
    requestMessage->setMethod(proxygen::HTTPMethod::GET);
    requestMessage->setURL("/");
    httpHandler.setResponseHandler(&responseHandler);
  }
};

TEST_F(StreamingFileHandlerTest, does_not_allow_reverse_traversal) {
  path inaccessible = tempDir.tempDir;
  inaccessible.remove_filename();
  inaccessible /= "testFile";
  ofstream fout(inaccessible.string());
  fout << "Data!" << endl;
  fout.close();

  httpHandler.setRequestArgs((path("..") / "testFile").string());

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(404, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, ignores_single_dots_in_paths) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "Data!" << endl;
  fout.close();

  httpHandler.setRequestArgs((path(".") / "testFile").string());

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(200, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Data!\n", to_string(responseHandler.bodies[0]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_404_on_missing_file) {
  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(404, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_404_on_permission_denied) {
  //TODO: This doesn't work for root, which I'm running as in a docker image
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "Data!" << endl;
  fout.close();

  fs::permissions(filename,
                  fs::remove_perms | fs::status(filename).permissions());
  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  fs::permissions(filename, fs::perms::owner_all);
  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(404, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_404_on_path_is_directory) {
  fs::create_directories(tempDir.tempDir / "testDir");

  httpHandler.setRequestArgs("testdir");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(404, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_304_on_file_not_changed) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "aaaaaaaaa" << endl;
  fout.close();

  requestMessage->getHeaders().set("If-Modified-Since",
                                   "Sat, 17 May 3000 07:07:39 GMT");
  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(304, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(0, responseHandler.bodies.size());
}
TEST_F(StreamingFileHandlerTest, does_not_return_304_on_invalid_date) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "Data!" << endl;
  fout.close();

  requestMessage->getHeaders().set("If-Modified-Since",
                                   "17 May 3000 07:07:39 GMT");
  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(200, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Data!\n", to_string(responseHandler.bodies[0]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_file_on_missing_expiry_header) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "Data!" << endl;
  fout.close();

  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(200, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Data!\n", to_string(responseHandler.bodies[0]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, returns_file_on_expired_expiry_header) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "Data!" << endl;
  fout.close();

  requestMessage->getHeaders().set("If-Modified-Since",
                                   "Wed, 17 May 2000 07:07:39 GMT");
  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(200, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(1, responseHandler.bodies.size());
  ASSERT_EQ("Data!\n", to_string(responseHandler.bodies[0]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST_F(StreamingFileHandlerTest, chunks_file) {
  auto filename = tempDir.tempDir / "testFile";
  ofstream fout(filename.string());
  fout << "aaaaaaaaa" << endl;
  fout << "bbbbbbbbb" << endl;
  fout << "cccc" << endl;
  fout.close();

  httpHandler.setRequestArgs("testFile");

  httpHandler.onRequest(std::move(requestMessage));
  httpHandler.onEOM();
  evb.loop();

  ASSERT_EQ(1, responseHandler.messages.size());
  ASSERT_EQ(200, responseHandler.messages[0].getStatusCode());
  ASSERT_EQ(3, responseHandler.bodies.size());
  ASSERT_EQ("aaaaaaaaa\n", to_string(responseHandler.bodies[0]));
  ASSERT_EQ("bbbbbbbbb\n", to_string(responseHandler.bodies[1]));
  ASSERT_EQ("cccc\n", to_string(responseHandler.bodies[2]));
  ASSERT_EQ(1, responseHandler.sendEOMCalls);
}

TEST(DISABLED_StreamingFileHandlerTest,
     returns_200_and_stops_processing_on_error) {}
}
}
