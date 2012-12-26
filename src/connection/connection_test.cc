/// @file
/// @author Alexander Thomson (thomson@cs.yale.edu)
/// @version 0.1
/// @since 5 April 2012
///
/// @section DESCRIPTION
///
/// Connection unit tests.
///
/// @TODO(alex): Write some tests spanning multiple physical machines.
/// @TODO(alex): All tests are commented out! Add them back in plz kthks.

#include "connection/connection.h"
#include "common/configuration.h"
#include "common/utils.h"
#include "connection/caravan.h"
#include <unistd.h>
#define GetCurrentDir getcwd

#include <gtest/gtest.h>
#include <glog/logging.h>

using caravan::ClusterManager;
using caravan::Machine;
using leveldb::Slice;

class ConnectionTest {
 public:
    ConnectionTest() {
       const char *channelText = "english";
       channel = new Slice(channelText);
    }

    ~ConnectionTest() {
      delete channel;
    }

    Slice DefaultChannel() {
      return *channel;
    }

 private:
  Slice *channel;
};



void TestConnection() {
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  EXPECT_FALSE(machine1.IsConnectedTo(0));
  EXPECT_FALSE(machine0.IsConnectedFrom(1));
  machine0.StartListening();
  machine1.Connect(0);
  EXPECT_TRUE(machine1.IsConnectedTo(0));
  EXPECT_TRUE(machine0.IsConnectedFrom(1));
  machine0.StopListening();
}

TEST(ConnectionTest, ShouldConnect) {
  TestConnection();
}

// Tests that sockets are reusable
TEST(ConnectionTest, ShouldConnectMultipleTimes) {
  TestConnection();
  TestConnection();
}

TEST(ConnectionTest, SendMessageOneNode) {
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  machine0.StartListening();
  machine1.Connect(0);
  const char *msgText = "Hello world!";
  Slice msg(msgText);
  const char *channelText = "english";
  Slice channel(channelText);
  machine1.SendMessage(0, channel, msg);
  caravan::Message *receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  machine0.StopListening();
}

TEST(ConnectionTest, ShouldReceiveNullMessageIfNothingSent) {
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  machine0.StartListening();
  machine1.Connect(0);
  EXPECT_EQ(NULL, machine0.ReceiveMessage());
  EXPECT_EQ(NULL, machine0.ReceiveMessage());
  machine0.StopListening();
}

TEST(ConnectionTest, SendMessageTwoNodes) {
  ConnectionTest t;
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  Machine machine2(2, 50002);

  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine0.AddMachine(2, "127.0.0.1", 50002);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  machine1.AddMachine(2, "127.0.0.1", 50002);
  machine2.AddMachine(0, "127.0.0.1", 50000);
  machine2.AddMachine(1, "127.0.0.1", 50001);

  machine0.StartListening();
  machine1.Connect(0);
  machine2.Connect(0);
  const char *msgText = "Hello world!";
  const char *msgText2 = "Goodbye world!";
  Slice msg(msgText);
  Slice msg2(msgText2);  
  machine1.SendMessage(0, t.DefaultChannel(), msg);
  machine2.SendMessage(0, t.DefaultChannel(), msg2);
  caravan::Message *receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  EXPECT_EQ(1, receivedMsg->GetSourceID());
  receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Goodbye world!", receivedMsg->data());
  EXPECT_EQ(2, receivedMsg->GetSourceID());
  machine0.StopListening();
}

TEST(ConnectionTest, ShouldAllowTwoNodesConnectingToEachOther) {
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  machine0.StartListening();
  machine1.StartListening();
  machine0.Connect(1);
  machine1.Connect(0);
  EXPECT_TRUE(machine0.IsConnectedTo(1));
  EXPECT_TRUE(machine0.IsConnectedFrom(1));
  EXPECT_TRUE(machine1.IsConnectedTo(0));
  EXPECT_TRUE(machine1.IsConnectedFrom(0));
  machine0.StopListening();
  machine1.StopListening();
}

TEST(ConnectionTest, ShouldAllowTwoNodesTalkingToEachOther) {
  ConnectionTest t;
  Machine machine0(0, 50000);
  Machine machine1(1, 50001);
  machine0.AddMachine(1, "127.0.0.1", 50001);
  machine1.AddMachine(0, "127.0.0.1", 50000);
  machine0.StartListening();
  machine1.StartListening();
  machine0.Connect(1);
  machine1.Connect(0);
  const char *msgText = "Hello world!";
  Slice msg(msgText);
  machine1.SendMessage(0, t.DefaultChannel(), msg);
  caravan::Message *receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  machine0.SendMessage(1, t.DefaultChannel(), msg);
  receivedMsg = machine1.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  machine0.StopListening();
  machine1.StopListening();
}

TEST(ConnectionTest, ShouldAllowMachineConfigConnect) {
  const char *file = "common/configuration_test.conf";
  MachineConfig config(file);
  Machine machine0(0, config);
  Machine machine1(1, config);
  EXPECT_FALSE(machine1.IsConnectedTo(0));
  EXPECT_FALSE(machine0.IsConnectedFrom(1));
  machine0.StartListening();
  machine1.Connect(0);
  EXPECT_TRUE(machine1.IsConnectedTo(0));
  EXPECT_TRUE(machine0.IsConnectedFrom(1));
  machine0.StopListening();
}


TEST(ConnectionTest, ShouldAllowMachineConfigMessages) {
  ConnectionTest t;
  const char *file = "common/configuration_test.conf";
  MachineConfig config(file);
  Machine machine0(0, config);
  Machine machine1(1, config);
  machine0.StartListening();
  machine1.Connect(0);
  const char *msgText = "Hello world!";
  Slice msg(msgText);
  machine1.SendMessage(0, t.DefaultChannel(), msg);
  caravan::Message *receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  machine0.StopListening();
}


TEST(ConnectionTest, ShouldUseChannel) {
  ConnectionTest t; 
  const char *file = "common/configuration_test.conf";
  MachineConfig config(file);
  Machine machine0(0, config);
  Machine machine1(1, config);
  machine0.StartListening();
  machine1.Connect(0);
  const char *msgText = "Hello world!";
  Slice msg(msgText);
  machine1.SendMessage(0, t.DefaultChannel(), msg);
  caravan::Message *receivedMsg = machine0.ReceiveMessage();
  EXPECT_STREQ("Hello world!", receivedMsg->data());
  EXPECT_STREQ("english", receivedMsg->channel());
  machine0.StopListening();
}


int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

