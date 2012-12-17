/// @file
/// @author Thaddeus Diamond <diamond@cs.yale.edu>
/// @version 0.1
/// @since 26 Jan 2012
///
/// @section DESCRIPTION
///
/// This is a TPC-C specific serializable argset
/// We use structs because, hey, ish is faster... got a problem?

#ifndef _DB_PROTO_TPCC_STRUCTS_H_
#define _DB_PROTO_TPCC_STRUCTS_H_

typedef struct Warehouse {
  // A warehouse has only one primary key
  char id[21];

  // The following are informational fields as defined in TPC-C standard
  char name[11];
  char street_1[21];
  char street_2[21];
  char city[21];
  char state[3];
  char zip[10];

  // The following are income records as specified in TPC-C standard
  double tax;
  double year_to_date;
} Warehouse;

typedef struct District {
  // A district has one primary key and one parent (foreign) key for the
  // warehouse it belongs to
  char id[21];
  char warehouse_id[21];

  // The following are informational fields for a district as defined by the
  // TPC-C standards
  char name[11];
  char street_1[21];
  char street_2[21];
  char city[21];
  char state[3];
  char zip[10];

  // The following are income records as specified in the TPC-C standard
  double tax;
  double year_to_date;
  int  next_order_id;
} District;

typedef struct Customer {
  // A customer has one primary key, one parent (foreign) key for the district
  // it belongs to and one grandparent (foreign) key for the warehouse the
  // district it is in belongs to
  char id[21];
  char district_id[21];
  char warehouse_id[21];

  // The following are informational fields for a customer as defined by the
  // TPC-C standards
  char first[21];
  char middle[21];
  char last[21];
  char street_1[21];
  char street_2[21];
  char city[21];
  char state[3];
  char zip[10];

  // The following is an data field for entering miscellany
  char data[501];

  // The following are income records as specified in the TPC-C standard
  int  since;
  char credit[3];
  double credit_limit;
  double discount;
  double balance;
  double year_to_date_payment;
  int  payment_count;
  int  delivery_count;
} Customer;

typedef struct NewOrder {
  // A new order has one primary key, one parent (foreign) key for the district
  // it originated in, and one grandparent (foreign) key for the warehouse
  // the district it originated in belongs to
  char id[21];
  char district_id[21];
  char warehouse_id[21];
} NewOrder;

typedef struct OrderLine {
  // An order line has a foreign key for the order it belongs to, the district
  // the order line occurs in, the warehouse that district belongs to,
  // which item is being ordered and which supply warehouse it is being
  // taken from
  char  order_id[21];
  char  district_id[21];
  char  warehouse_id[21];
  char  item_id[21];
  char  supply_warehouse_id[21];

  // The following are informational fields for an orderline as defined by the
  // TPC-C standards
  char   district_information[50];
  int    number;
  double delivery_date;
  int    quantity;
  double amount;
} OrderLine;

typedef struct Order {
  // An order has one primary key, one parent (foreign) key for the customer
  // that originated the order, one grandparent (foreign) key for the district
  // that customer is in, and one grandparent (foreign) key for the district's
  // warehouse
  char id[21];
  char district_id[21];
  char warehouse_id[21];
  char customer_id[21];

  // A set of order lines
  OrderLine order_lines[15];

  // The following are informational fields for an order as defined by the
  // TPC-C standards
  double entry_date;
  int carrier_id;
  int order_line_count;
  bool all_items_local;
} Order;

typedef struct Item {
  // An item has only one primary key
  char id[21];

  // The following is an data field for entering miscellany
  char data[501];

  // The following are informational fields for an item as defined by the
  // TPC-C standards
  char name[25];
  double price;
} Item;

typedef struct Stock {
  // A stock has one primary key (the item it represents) and one
  // foreign key (the warehouse it is in)
  char id[21];
  char item_id[21];
  char warehouse_id[21];

  // The following is an data field for entering miscellany
  char data[501];

  // The following are informational fields for a stock as defined by the
  // TPC-C standards
  int  quantity;
  char districts[21][20];
  int  year_to_date;
  int  order_count;
  int  remote_count;
} Stock;

typedef struct History {
  // A history object contains keys for the customer that originated the
  // item, which district and warehouse it was in, and which district and
  // warehouse the customer belonged to
  char customer_id[21];
  char district_id[21];
  char warehouse_id[21];
  char customer_district_id[21];
  char customer_warehouse_id[21];

  // The following is an data field for entering miscellany
  char data[501];

  // The following are informational fields for a history as defined by the
  // TPC-C standards
  double date;
  double amount;
} History;

#endif  // _DB_PROTO_TPCC_STRUCTS_H_
