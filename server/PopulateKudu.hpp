/*
 * (C) Copyright 2015 ETH Zurich Systems Group (http://www.systems.ethz.ch/) and others.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Contributors:
 *     Markus Pilman <mpilman@inf.ethz.ch>
 *     Simon Loesing <sloesing@inf.ethz.ch>
 *     Thomas Etter <etterth@gmail.com>
 *     Kevin Bocksrocker <kevin.bocksrocker@gmail.com>
 *     Lucas Braun <braunl@inf.ethz.ch>
 */
#pragma once
#include <random>
#include <cstdint>
#include <crossbow/string.hpp>
#include <common/Util.hpp>

#include <kudu/client/client.h>

namespace tpcc {

class Populator {
    Random_t& mRandom;
    crossbow::string mOriginal = "ORIGINAL";
public:
    Populator() : mRandom(*Random()) {}
    void populateDimTables(kudu::client::KuduSession& transaction, bool useCH);
    void populateWarehouse(kudu::client::KuduSession& transaction, int16_t w_id, bool useCH);
private:
    void populateItems(kudu::client::KuduSession& transaction);
    void populateRegions(kudu::client::KuduSession& transaction);
    void populateNations(kudu::client::KuduSession& transaction);
    void populateSuppliers(kudu::client::KuduSession &transaction);
    void populateStocks(kudu::client::KuduSession& transaction, int16_t w_id, bool useCH);
    void populateDistricts(kudu::client::KuduSession& transaction, int16_t w_id, bool useCH);
    void populateCustomers(kudu::client::KuduSession& transaction, int16_t w_id, int16_t d_id, int64_t c_since, bool useCH);
    void populateHistory(kudu::client::KuduSession& transaction, int32_t c_id, int16_t d_id, int16_t w_id, int64_t n);
    void populateOrders(kudu::client::KuduSession& transaction, int16_t d_id, int16_t w_id, int64_t o_entry_d);
    void populateOrderLines(kudu::client::KuduSession& transaction,
            int32_t o_id, int16_t d_id, int16_t w_id, int16_t ol_cnt, int64_t o_entry_d);
    void populateNewOrders(kudu::client::KuduSession& transaction, int16_t w_id, int16_t d_id);
};

} // namespace tpcc
