/// @file
/// @author Thaddeus Diamond  <diamond@cs.yale.edu>
/// @author Alexander Thomson <thomson@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// The Application abstract class

#ifndef _DB_STORED_PROCEDURES_APPLICATION_H_
#define _DB_STORED_PROCEDURES_APPLICATION_H_

#define MAX_KEY_LENGTH 128

#include <string>

#include "backend/storage.h"
#include "backend/transactional_storage_manager.h"
#include "common/types.h"

class Configuration;
class TxnProto;

/// @enum  TxnStatus
/// @brief Report on whether the txn succeeded, failed or needs to be redone
enum TxnStatus {
  SUCCESS = 0,
  FAILURE,
  REDO,
  TOTAL_TXN_STATUS_TYPES,
};

namespace calvin {

using std::string;

/// @class Application
/// @brief This defines an arbitrary application the DB is executing
///
/// We want to allow any arbitrary application (i.e. TPC-C, microbenchmark),
/// to be able to execute in Calvin.  In order to do this, the application
/// must define several standard interfaces that our Calvin backend uses to
/// initialize, generate a transactional load, and execute.
///
/// @attention  The Application class follows the Singleton DP and is complex
class Application {
 public:
  /// The Application destructor must free all aggregated memory, although
  /// since there is no internal state in an application, this should be
  /// null.
  virtual ~Application() {}

  /// We provide a human-readable name function for each class
  virtual const char* Name() const = 0;

  /// NewTxn() is responsible for generating a transactional load in the app
  ///
  /// @param    txn_id    The id of the next transaction to be generated
  /// @param    txn_type  The type of the txn to be generated, app-specific
  /// @param    args      A serialized ProtoBuf representing app-specific args
  ///
  /// @returns  TxnProto (pointer to ProtoBuf) representing the txn
  virtual TxnProto* NewTxn(int64 txn_id, int txn_type, string args) const = 0;

  /// Executes a transaction's application logic given the input 'txn'
  ///
  /// @param    txn       The transaction object to be executed
  /// @param    storage   The TSM interface for the transaction to use
  ///
  /// @returns  A status code as enumerated in @ref TxnStatus
  virtual TxnStatus Execute(TxnProto* txn,
                            TransactionalStorageManager* storage) const = 0;

  /// The following initializes the database with whatever is specified by
  /// a given application
  virtual void InitializeStorage() const = 0;

  /// Every method must have a dictionary hashing method for turning a string
  /// key into an integer.  Support is given for up to \ref NUMBER_OF_TXNS
  ///
  /// @param    key       The key to be converted
  ///
  /// @returns  An integer that is a dictionary hash of the key
  virtual int KeyToInt(const Key& key) const = 0;

  /// Return the size of main memory (for use in Ping-Pong)
  ///
  /// @returns  An integer that represents the size of DB in main memory
  virtual int DBSize() const = 0;

  /// @enum   ApplicationType
  /// @brief  The available stored_procedures in our system
  ///
  /// This is a list of all possible stored_procedures that can be run on Calvin
  enum ApplicationType {
    MICROBENCHMARK = 0,
    TPCC,
  };

  /// A la Singleton DP, we create a static way to return the sole instance
  static Application* GetApplicationInstance();

 protected:
  /// Following the Singleton DP, we have a static private var
  static Application* application_instance_;

  /// The constructor for an application is protected so that child classes
  /// can initialize it
  Application();
};

}  // namespace calvin

#endif  // _DB_STORED_PROCEDURES_APPLICATION_H_
