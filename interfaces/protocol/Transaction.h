/*
 *  Copyright (C) 2021 FISCO BCOS.
 *  SPDX-License-Identifier: Apache-2.0
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 * @brief interfaces for transaction
 * @file Transaction.h
 */
#pragma once
#include "../../interfaces/crypto/CryptoSuite.h"
#include "../../libutilities/Common.h"
#include "../../libutilities/Error.h"
#include "TransactionSubmitResult.h"
namespace bcos
{
namespace protocol
{
enum TransactionType
{
    NullTransaction,
    ContractCreation,
    MessageCall,
};
using TxSubmitCallback =
    std::function<void(Error::Ptr, bcos::protocol::TransactionSubmitResult::Ptr)>;
class Transaction
{
public:
    using Ptr = std::shared_ptr<Transaction>;
    using ConstPtr = std::shared_ptr<const Transaction>;
    explicit Transaction(bcos::crypto::CryptoSuite::Ptr _cryptoSuite) : m_cryptoSuite(_cryptoSuite)
    {}

    virtual ~Transaction() {}

    virtual void decode(bytesConstRef _txData, bool _checkSig) = 0;
    virtual void encode(bytes& _encodedData) const = 0;
    virtual bytesConstRef encode(bool _onlyHashFields = false) const = 0;
    virtual bcos::crypto::HashType const& hash() const
    {
        UpgradableGuard l(x_hash);
        if (m_hash != bcos::crypto::HashType())
        {
            return m_hash;
        }
        auto hashFields = encode(true);
        UpgradeGuard ul(l);
        m_hash = m_cryptoSuite->hash(hashFields);
        return m_hash;
    }

    virtual void verify() const
    {
        // The tx has already been verified
        if (sender().size() > 0)
        {
            return;
        }
        // check the signatures
        auto signature = signatureData();
        auto publicKey = m_cryptoSuite->signatureImpl()->recover(hash(), signature);
        // recover the sender
        forceSender(m_cryptoSuite->calculateAddress(publicKey).asBytes());
    }

    virtual int32_t version() const = 0;
    virtual std::string_view chainId() const = 0;
    virtual std::string_view groupId() const = 0;
    virtual int64_t blockLimit() const = 0;
    virtual u256 const& nonce() const = 0;
    virtual bytesConstRef to() const = 0;
    virtual bytesConstRef sender() const { return bytesConstRef(m_sender.data(), m_sender.size()); }

    virtual bytesConstRef input() const = 0;
    virtual int64_t importTime() const = 0;
    virtual TransactionType type() const = 0;
    virtual void forceSender(bytes const& _sender) const { m_sender = _sender; }

    virtual bytesConstRef signatureData() const = 0;

    virtual TxSubmitCallback submitCallback() const { return m_submitCallback; }
    virtual void setSubmitCallback(TxSubmitCallback _submitCallback)
    {
        m_submitCallback = _submitCallback;
    }
    virtual bool synced() const { return m_synced; }
    virtual void setSynced(bool _synced) const { m_synced = _synced; }

    virtual bool sealed() const { return m_sealed; }
    virtual void setSealed(bool _sealed) const { m_sealed = _sealed; }

    virtual bool invalid() const { return m_invalid; }
    virtual void setInvalid(bool _invalid) const { m_invalid = _invalid; }

    virtual bcos::crypto::CryptoSuite::Ptr cryptoSuite() { return m_cryptoSuite; }

protected:
    mutable bcos::bytes m_sender = bcos::bytes();
    mutable bcos::crypto::HashType m_hash = bcos::crypto::HashType();
    mutable SharedMutex x_hash;
    bcos::crypto::CryptoSuite::Ptr m_cryptoSuite;

    TxSubmitCallback m_submitCallback;
    // the tx has been synced or not
    mutable bool m_synced = false;
    // the tx has been sealed by the leader of not
    mutable bool m_sealed = false;
    // the tx is invalid for verify failed
    mutable bool m_invalid = false;
};

using Transactions = std::vector<Transaction::Ptr>;
using TransactionsPtr = std::shared_ptr<Transactions>;
using TransactionsConstPtr = std::shared_ptr<const Transactions>;
using ConstTransactions = std::vector<Transaction::ConstPtr>;
using ConstTransactionsPtr = std::shared_ptr<ConstTransactions>;

}  // namespace protocol
}  // namespace bcos