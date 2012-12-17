/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 4 Dec 2011
///
/// @section DESCRIPTION
///
/// This abstract factory provides a standardized way to generate black boxes
/// for use in unit tests.

#ifndef _DB_COMMON_BLACK_BOX_FACTORY_H_
#define _DB_COMMON_BLACK_BOX_FACTORY_H_

#include "backend/storage.h"
#include "backend/transactional_storage_manager.h"
#include "checkpointing/naive.h"
#include "common/configuration.h"
#include "connection/connection.h"

namespace calvin {

/// @class BlackBoxFactory
/// @brief This defines a factory to produce arbitrary classes
///
/// In order to make the unit tests attached to the code true unit tests they
/// cannot rely on other parts of the code.  Therefore, we use this class
/// to statically generate arbitrary black boxes.
///
/// @attention
/// Every interface or absolute superclass must have a method contained herein
/// to generate a black box.
class BlackBoxFactory {
 public:
  /// Every interface must, first and foremost, implement a UnitTestFactory()
  /// generator to allow black box unit tests and avoid integration tests.

  static Storage* StorageBlackBox(Checkpointer* decorator = NULL) {
    Storage::DestroyStorage();
    if (decorator)
      Storage::InitializeStorage(decorator, NULL);
    return Storage::GetStorageInstance();
  }

  static TransactionalStorageManager* TransactionalStorageManagerBlackBox(
      TxnProto* txn, bool overwrite = true) {
    if (overwrite)
      StorageBlackBox();

    return new TransactionalStorageManager(ConnectionBlackBox(), txn);
  }

  static Checkpointer* CheckpointingBlackBox() {
    return new NaiveCheckpointer();
  }

  static Config* ConfigurationBlackBox() {
    if (!Config::GetConfigInstance())
      Config::LoadConfiguration(0, "common/configuration_test_one_node.conf");
    return Config::GetConfigInstance();
  }

  static ConnectionMultiplexer* ConnectionMultiplexerBlackBox() {
    return new ConnectionMultiplexer();
  }

  static Connection* ConnectionBlackBox() {
    return ConnectionMultiplexerBlackBox()->NewConnection("asdf");
  }

 private:
  /// Using the Factory design pattern the constructor must be left private
  BlackBoxFactory() {}

  /// Using the Factory design pattern the constructor must be left private
  ~BlackBoxFactory() {}
};

}  // namespace calvin

#endif  // _DB_COMMON_BLACK_BOX_FACTORY_H_
