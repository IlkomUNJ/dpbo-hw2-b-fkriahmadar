#include "state.h"

// Definitions for variables declared extern in state.h
std::vector<Account> accounts;
std::vector<std::vector<Item>> inventories;
std::vector<BankAccount> bankAccounts;
std::vector<std::vector<Order>> sellerOrders;
std::vector<std::vector<Order>> buyerOrders;
int nextOrderId = 1;
std::vector<Patient> patients;