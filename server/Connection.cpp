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
#include "Connection.hpp"
#include "CreateSchema.hpp"
#include "Populate.hpp"
#include "Transactions.hpp"

#include <telldb/Transaction.hpp>

using namespace boost::asio;

namespace tpcc {

class CommandImpl {
    Connection* mConnection;
    server::Server<CommandImpl> mServer;
    boost::asio::io_service& mService;
    tell::db::ClientManager<void>& mClientManager;
    std::unique_ptr<tell::db::TransactionFiber<void>> mFiber;
    Transactions mTransactions;
public:
    CommandImpl(Connection* connection,
            boost::asio::ip::tcp::socket& socket,
            boost::asio::io_service& service,
            tell::db::ClientManager<void>& clientManager,
            int16_t numWarehouses)
        : mConnection(connection)
        , mServer(*this, socket)
        , mService(service)
        , mClientManager(clientManager)
        , mTransactions(numWarehouses)
    {}

    void run() {
        mServer.run();
    }

    void close() {
        delete mConnection;
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::EXIT, void>::type
    execute(const Callback callback) {
        mServer.quit();
        callback();
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::CREATE_SCHEMA, void>::type
    execute(std::tuple<int16_t, bool> args, const Callback& callback) {
        bool ch = std::get<1>(args);
        auto transaction = [this, ch, callback](tell::db::Transaction& tx){
            bool success;
            crossbow::string msg;
            try {
                createSchema(tx, ch);
                tx.commit();
                success = true;
            } catch (std::exception& ex) {
                tx.rollback();
                success = false;
                msg = ex.what();
            }
            mService.post([this, callback, success, msg](){
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(std::make_tuple(success, msg));
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::POPULATE_WAREHOUSE, void>::type
    execute(std::tuple<int16_t, bool> args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            bool success;
            crossbow::string msg;
            try {
                auto counter = tx.getCounter("history_counter");
                Populator populator;
                populator.populateWarehouse(tx, counter, std::get<0>(args), std::get<1>(args));
                tx.commit();
                success = true;
            } catch (std::exception& ex) {
                tx.rollback();
                success = false;
                msg = ex.what();
            }
            mService.post([this, success, msg, callback](){
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(std::make_pair(success, msg));
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::POPULATE_DIM_TABLES, void>::type
    execute(bool args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            bool success;
            crossbow::string msg;
            try {
                Populator populator;
                populator.populateDimTables(tx, args);
                tx.commit();
                success = true;
            } catch (std::exception& ex) {
                tx.rollback();
                success = false;
                msg = ex.what();
            }
            mService.post([this, success, msg, callback](){
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(std::make_pair(success, msg));
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::NEW_ORDER, void>::type
    execute(const typename Signature<C>::arguments& args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            typename Signature<C>::result res = mTransactions.newOrderTransaction(tx, args);
            mService.post([this, res, callback]() {
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(res);
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::PAYMENT, void>::type
    execute(const typename Signature<C>::arguments& args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            typename Signature<C>::result res = mTransactions.payment(tx, args);
            mService.post([this, res, callback]() {
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(res);
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::ORDER_STATUS, void>::type
    execute(const typename Signature<C>::arguments& args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            typename Signature<C>::result res = mTransactions.orderStatus(tx, args);
            mService.post([this, res, callback]() {
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(res);
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::DELIVERY, void>::type
    execute(const typename Signature<C>::arguments& args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            typename Signature<C>::result res = mTransactions.delivery(tx, args);
            mService.post([this, res, callback]() {
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(res);
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction)));
    }

    template<Command C, class Callback>
    typename std::enable_if<C == Command::STOCK_LEVEL, void>::type
    execute(const typename Signature<C>::arguments& args, const Callback& callback) {
        auto transaction = [this, args, callback](tell::db::Transaction& tx) {
            typename Signature<C>::result res = mTransactions.stockLevel(tx, args);
            mService.post([this, res, callback]() {
                mFiber->wait();
                mFiber.reset(nullptr);
                callback(res);
            });
        };
        mFiber.reset(new tell::db::TransactionFiber<void>(mClientManager.startTransaction(transaction,
                        tell::store::TransactionType::READ_ONLY)));
    }
};

Connection::Connection(boost::asio::io_service& service, tell::db::ClientManager<void>& clientManager, int16_t numWarehouses)
    : mSocket(service)
    , mImpl(new CommandImpl(this, mSocket, service, clientManager, numWarehouses))
{}

Connection::~Connection() = default;

void Connection::run() {
    mImpl->run();
}

} // namespace tpcc

