#pragma once
#include "bank_customer.h"
#include <string>
#include <vector>

class Bank {
private:
    std::string name;
    std::vector<BankCustomer> Accounts;
    int customerCount;

public:
    Bank(const std::string& name);
    void addCustomer(const BankCustomer& customer);
    void printCustomers() const;
    int getCustomerCount() const;
};