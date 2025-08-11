// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2021-2023 The Dogecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "clientversion.h"
#include "init.h"
#include "validation.h"
#include "net.h"
#include <txmempool.h>
#include <consensus/consensus.h>
#include "netbase.h"
#include "rpc/server.h"
#include "timedata.h"
#include "util.h"
#include "utilstrencodings.h"
#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#endif

#include <stdint.h>

#include <boost/assign/list_of.hpp>

#include <univalue.h>

using namespace std;

/**
 * @note Do not add or change anything in the information returned by this
 * method. `getinfo` exists for backwards-compatibility only. It combines
 * information from wildly different sources in the program, which is a mess,
 * and is thus planned to be deprecated eventually.
 *
 * Based on the source of the information, new information should be added to:
 * - `getblockchaininfo`,
 * - `getnetworkinfo` or
 * - `getwalletinfo`
 *
 * Or alternatively, create a specific query method for the information.
 **/
UniValue getinfo(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "\nDEPRECATED. Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total shibacoin balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
            "  \"testnet\": true|false,      (boolean) if the server is using testnet or not\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since Unix epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in " + CURRENCY_UNIT + "/kB\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for non-free transactions in " + CURRENCY_UNIT + "/kB\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : NULL);
#else
    LOCK(cs_main);
#endif

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("version", CLIENT_VERSION);
    obj.pushKV("protocolversion", PROTOCOL_VERSION);
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.pushKV("walletversion", pwalletMain->GetVersion());
        obj.pushKV("balance",       ValueFromAmount(pwalletMain->GetBalance()));
    }
#endif
    obj.pushKV("blocks",        (int)chainActive.Height());
    obj.pushKV("timeoffset",    GetTimeOffset());
    if(g_connman)
        obj.pushKV("connections",   (int)g_connman->GetNodeCount(CConnman::CONNECTIONS_ALL));
    obj.pushKV("proxy",         (proxy.IsValid() ? proxy.proxy.ToStringIPPort() : string()));
    obj.pushKV("difficulty",    (double)GetDifficulty());
    obj.pushKV("testnet",       Params().NetworkIDString() == CBaseChainParams::TESTNET);
#ifdef ENABLE_WALLET
    if (pwalletMain) {
        obj.pushKV("keypoololdest", pwalletMain->GetOldestKeyPoolTime());
        obj.pushKV("keypoolsize",   (int)pwalletMain->GetKeyPoolSize());
    }
    if (pwalletMain && pwalletMain->IsCrypted())
        obj.pushKV("unlocked_until", nWalletUnlockTime);
    obj.pushKV("paytxfee",      ValueFromAmount(payTxFee.GetFeePerK()));
#endif
    obj.pushKV("relayfee",      ValueFromAmount(::minRelayTxFeeRate.GetFeePerK()));
    obj.pushKV("errors",        GetWarnings("statusbar"));
    return obj;
}

#ifdef ENABLE_WALLET
class DescribeAddressVisitor : public boost::static_visitor<UniValue>
{
public:
    UniValue operator()(const CNoDestination &dest) const { return UniValue(UniValue::VOBJ); }

    UniValue operator()(const CKeyID &keyID) const {
        UniValue obj(UniValue::VOBJ);
        CPubKey vchPubKey;
        obj.pushKV("isscript", false);
        if (pwalletMain && pwalletMain->GetPubKey(keyID, vchPubKey)) {
            obj.pushKV("pubkey", HexStr(vchPubKey));
            obj.pushKV("iscompressed", vchPubKey.IsCompressed());
        }
        return obj;
    }

    UniValue operator()(const CScriptID &scriptID) const {
        UniValue obj(UniValue::VOBJ);
        CScript subscript;
        obj.pushKV("isscript", true);
        if (pwalletMain && pwalletMain->GetCScript(scriptID, subscript)) {
            std::vector<CTxDestination> addresses;
            txnouttype whichType;
            int nRequired;
            ExtractDestinations(subscript, whichType, addresses, nRequired);
            obj.pushKV("script", GetTxnOutputType(whichType));
            obj.pushKV("hex", HexStr(subscript.begin(), subscript.end()));
            UniValue a(UniValue::VARR);
            BOOST_FOREACH(const CTxDestination& addr, addresses)
                a.push_back(CBitcoinAddress(addr).ToString());
            obj.pushKV("addresses", a);
            if (whichType == TX_MULTISIG)
                obj.pushKV("sigsrequired", nRequired);
        }
        return obj;
    }
};
#endif

UniValue validateaddress(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "validateaddress \"address\"\n"
            "\nReturn information about the given shibacoin address.\n"
            "\nArguments:\n"
            "1. \"address\"     (string, required) The shibacoin address to validate\n"
            "\nResult:\n"
            "{\n"
            "  \"isvalid\" : true|false,       (boolean) If the address is valid or not. If not, this is the only property returned.\n"
            "  \"address\" : \"address\", (string) The shibacoin address validated\n"
            "  \"scriptPubKey\" : \"hex\",       (string) The hex encoded scriptPubKey generated by the address\n"
            "  \"ismine\" : true|false,        (boolean) If the address is yours or not\n"
            "  \"iswatchonly\" : true|false,   (boolean) If the address is watchonly\n"
            "  \"isscript\" : true|false,      (boolean) If the key is a script\n"
            "  \"pubkey\" : \"publickeyhex\",    (string) The hex value of the raw public key\n"
            "  \"iscompressed\" : true|false,  (boolean) If the address is compressed\n"
            "  \"account\" : \"account\"         (string) DEPRECATED. The account associated with the address, \"\" is the default account\n"
            "  \"timestamp\" : timestamp,        (number, optional) The creation time of the key if available in seconds since epoch (Jan 1 1970 GMT)\n"
            "  \"hdkeypath\" : \"keypath\"       (string, optional) The HD keypath if the key is HD and available\n"
            "  \"hdmasterkeyid\" : \"<hash160>\" (string, optional) The Hash160 of the HD master pubkey\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddress", "\"PsaCGTi7WgYtg6kEeNb9fayaycJVC7i9UL\"")
            + HelpExampleRpc("validateaddress", "\"PsaCGTi7WgYtg6kEeNb9fayaycJVC7i9UL\"")
        );

#ifdef ENABLE_WALLET
    LOCK2(cs_main, pwalletMain ? &pwalletMain->cs_wallet : NULL);
#else
    LOCK(cs_main);
#endif

    CBitcoinAddress address(request.params[0].get_str());
    bool isValid = address.IsValid();

    UniValue ret(UniValue::VOBJ);
    ret.pushKV("isvalid", isValid);
    if (isValid)
    {
        CTxDestination dest = address.Get();
        string currentAddress = address.ToString();
        ret.pushKV("address", currentAddress);

        CScript scriptPubKey = GetScriptForDestination(dest);
        ret.pushKV("scriptPubKey", HexStr(scriptPubKey.begin(), scriptPubKey.end()));

#ifdef ENABLE_WALLET
        isminetype mine = pwalletMain ? IsMine(*pwalletMain, dest) : ISMINE_NO;
        ret.pushKV("ismine", (mine & ISMINE_SPENDABLE) ? true : false);
        ret.pushKV("iswatchonly", (mine & ISMINE_WATCH_ONLY) ? true: false);
        UniValue detail = boost::apply_visitor(DescribeAddressVisitor(), dest);
        ret.pushKVs(detail);
        if (pwalletMain && pwalletMain->mapAddressBook.count(dest))
            ret.pushKV("account", pwalletMain->mapAddressBook[dest].name);
        CKeyID keyID;
        if (pwalletMain) {
            const auto& meta = pwalletMain->mapKeyMetadata;
            auto it = address.GetKeyID(keyID) ? meta.find(keyID) : meta.end();
            if (it == meta.end()) {
                it = meta.find(CScriptID(scriptPubKey));
            }
            if (it != meta.end()) {
                ret.pushKV("timestamp", it->second.nCreateTime);
                if (!it->second.hdKeypath.empty()) {
                    ret.pushKV("hdkeypath", it->second.hdKeypath);
                    ret.pushKV("hdmasterkeyid", it->second.hdMasterKeyID.GetHex());
                }
            }
        }
#endif
    }
    return ret;
}

/**
 * Used by addmultisigaddress / createmultisig:
 */
CScript _createmultisig_redeemScript(const UniValue& params)
{
    int nRequired = params[0].get_int();
    const UniValue& keys = params[1].get_array();

    // Gather public keys
    if (nRequired < 1)
        throw runtime_error("a multisignature address must require at least one key to redeem");
    if ((int)keys.size() < nRequired)
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)", keys.size(), nRequired));
    if (keys.size() > 16)
        throw runtime_error("Number of addresses involved in the multisignature address creation > 16\nReduce the number");
    std::vector<CPubKey> pubkeys;
    pubkeys.resize(keys.size());
    for (unsigned int i = 0; i < keys.size(); i++)
    {
        const std::string& ks = keys[i].get_str();
#ifdef ENABLE_WALLET
        // Case 1: Bitcoin address and we have full public key:
        CBitcoinAddress address(ks);
        if (pwalletMain && address.IsValid())
        {
            CKeyID keyID;
            if (!address.GetKeyID(keyID))
                throw runtime_error(
                    strprintf("%s does not refer to a key",ks));
            CPubKey vchPubKey;
            if (!pwalletMain->GetPubKey(keyID, vchPubKey))
                throw runtime_error(
                    strprintf("no full public key for address %s",ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }

        // Case 2: hex public key
        else
#endif
        if (IsHex(ks))
        {
            CPubKey vchPubKey(ParseHex(ks));
            if (!vchPubKey.IsFullyValid())
                throw runtime_error(" Invalid public key: "+ks);
            pubkeys[i] = vchPubKey;
        }
        else
        {
            throw runtime_error(" Invalid public key: "+ks);
        }
    }
    CScript result = GetScriptForMultisig(nRequired, pubkeys);

    if (result.size() > MAX_SCRIPT_ELEMENT_SIZE)
        throw runtime_error(
                strprintf("redeemScript exceeds size limit: %d > %d", result.size(), MAX_SCRIPT_ELEMENT_SIZE));

    return result;
}

static bool getAddressesFromParams(const UniValue& params, std::vector<std::pair<uint160, int> > &addresses)
 {
     if (params[0].isStr()) {
         CBitcoinAddress address(params[0].get_str());
         CTxDestination dest = address.Get();
         CScript scriptPubKey = GetScriptForDestination(dest);
         uint160 hashBytes;
         int addressType = 0;
 
         if (scriptPubKey.IsPayToScriptHash()) {
             hashBytes = uint160(std::vector <unsigned char>(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22));
             addressType = 2;
         } else if (scriptPubKey.IsPayToPublicKeyHash()) {
             hashBytes = uint160(std::vector <unsigned char>(scriptPubKey.begin() + 3, scriptPubKey.begin() + 23));
             addressType = 1;
         } else if (scriptPubKey.IsPayToWitnessPubkeyHash()) {
             hashBytes = uint160(std::vector <unsigned char>(scriptPubKey.begin() + 2, scriptPubKey.end()));
             addressType = 1;
         } else if (scriptPubKey.IsPayToWitnessScriptHash()) {
             hashBytes = Hash160(std::vector <unsigned char> (scriptPubKey.begin() + 2, scriptPubKey.end()));
             addressType = 2;
         } else {
             hashBytes.SetNull();
             addressType = 0;
         }
 
         if (addressType == 0) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
         }
 
         addresses.push_back(std::make_pair(hashBytes, addressType));
     } else {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     return true;
 }
 
 bool heightSort(std::pair<CAddressUnspentKey, CAddressUnspentValue> a,
                 std::pair<CAddressUnspentKey, CAddressUnspentValue> b) {
     return a.second.blockHeight < b.second.blockHeight;
 }
 
 bool timestampSort(std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> a,
                    std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> b) {
     return a.second.time < b.second.time;
 }
 
 bool getAddressFromIndex(const int &type, const uint160 &hash, std::string &address)
 {
     if (type == 2) {
         address = CBitcoinAddress(CScriptID(hash)).ToString();
     } else if (type == 1) {
         address = CBitcoinAddress(CKeyID(hash)).ToString();
     } else {
         return false;
     }
     return true;
 }
 
 UniValue getaddressdeltas(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
         throw std::runtime_error(
             "getaddressdeltas\n"
             "\nReturns all changes for an address (requires addressindex to be enabled).\n"
             "\nArguments:\n"
             "{\n"
             "  \"addresses\"\n"
             "    [\n"
             "      \"address\"  (string) The base58check encoded address\n"
             "      ,...\n"
             "    ]\n"
             "  \"start\" (number) The start block height\n"
             "  \"end\" (number) The end block height\n"
             "  \"chainInfo\" (boolean) Include chain info in results, only applies if start and end specified\n"
             "}\n"
             "\nResult:\n"
             "[\n"
             "  {\n"
             "    \"satoshis\"  (number) The difference of satoshis\n"
             "    \"txid\"  (string) The related txid\n"
             "    \"index\"  (number) The related input or output index\n"
             "    \"height\"  (number) The block height\n"
             "    \"address\"  (string) The base58check encoded address\n"
             "  }\n"
             "]\n"
             "\nExamples:\n"
             + HelpExampleCli("getaddressdeltas", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
             + HelpExampleRpc("getaddressdeltas", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
         );
 
 
     UniValue startValue = find_value(request.params[0].get_obj(), "start");
     UniValue endValue = find_value(request.params[0].get_obj(), "end");
 
     UniValue chainInfo = find_value(request.params[0].get_obj(), "chainInfo");
     bool includeChainInfo = false;
     if (chainInfo.isBool()) {
         includeChainInfo = chainInfo.get_bool();
     }
 
     int start = 0;
     int end = 0;
 
     if (startValue.isNum() && endValue.isNum()) {
         start = startValue.get_int();
         end = endValue.get_int();
         if (start <= 0 || end <= 0) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start and end is expected to be greater than zero");
         }
         if (end < start) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "End value is expected to be greater than start");
         }
     }
 
     std::vector<std::pair<uint160, int> > addresses;
 
     if (!getAddressesFromParams(request.params, addresses)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
 
     for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
         if (start > 0 && end > 0) {
             if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                 throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
             }
         } else {
             if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                 throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
             }
         }
     }
 
     UniValue deltas(UniValue::VARR);
 
     for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
         std::string address;
         if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
         }
 
         UniValue delta(UniValue::VOBJ);
         delta.pushKV("satoshis", it->second);
         delta.pushKV("txid", it->first.txhash.GetHex());
         delta.pushKV("index", (int)it->first.index);
         delta.pushKV("blockindex", (int)it->first.txindex);
         delta.pushKV("height", it->first.blockHeight);
         delta.pushKV("address", address);
         deltas.push_back(delta);
     }
 
     UniValue result(UniValue::VOBJ);
 
     if (includeChainInfo && start > 0 && end > 0) {
         LOCK(cs_main);
 
         if (start > chainActive.Height() || end > chainActive.Height()) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Start or end is outside chain range");
         }
 
         CBlockIndex* startIndex = chainActive[start];
         CBlockIndex* endIndex = chainActive[end];
 
         UniValue startInfo(UniValue::VOBJ);
         UniValue endInfo(UniValue::VOBJ);
 
         startInfo.pushKV("hash", startIndex->GetBlockHash().GetHex());
         startInfo.pushKV("height", start);
 
         endInfo.pushKV("hash", endIndex->GetBlockHash().GetHex());
         endInfo.pushKV("height", end);
 
         result.pushKV("deltas", deltas);
         result.pushKV("start", startInfo);
         result.pushKV("end", endInfo);
 
         return result;
     } else {
         return deltas;
     }
 }
 
 UniValue getaddressbalance(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() != 1)
         throw std::runtime_error(
             "getaddressbalance\n"
             "\nReturns the balance for an address(es) (requires addressindex to be enabled).\n"
             "\nArguments:\n"
             "{\n"
             "  \"addresses\"\n"
             "    [\n"
             "      \"address\"  (string) The base58check encoded address\n"
             "      ,...\n"
             "    ]\n"
             "}\n"
             "\nResult:\n"
             "{\n"
             "  \"balance\"  (string) The current balance in satoshis\n"
             "  \"received\"  (string) The total number of satoshis received (including change)\n"
             "}\n"
             "\nExamples:\n"
             + HelpExampleCli("getaddressbalance", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
             + HelpExampleRpc("getaddressbalance", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
         );
 
     std::vector<std::pair<uint160, int> > addresses;
 
     if (!getAddressesFromParams(request.params, addresses)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
 
     for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
         if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
         }
     }
 
     CAmount balance = 0;
     CAmount received = 0;
 
     for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
         if (it->second > 0) {
             received += it->second;
         }
         balance += it->second;
     }
 
     UniValue result(UniValue::VOBJ);
     result.pushKV("balance", balance);
     result.pushKV("received", received);
 
     return result;
 }
 
 UniValue getaddressutxos(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() < 1 || request.params.size() > 3)
         throw std::runtime_error(
             "getaddressutxos\n"
             "\nReturns all unspent outputs for an address (requires addressindex to be enabled).\n"
             "\nArguments:\n"
             "{\n"
             "  \"addresses\"\n"
             "    [\n"
             "      \"address\"  (string) The base58check encoded address\n"
             "      ,...\n"
             "    ],\n"
             "  \"chainInfo\"  (boolean) Include chain info with results\n"
             "}\n"
             "\nResult\n"
             "[\n"
             "  {\n"
             "    \"address\"  (string) The address base58check encoded\n"
             "    \"txid\"  (string) The output txid\n"
             "    \"height\"  (number) The block height\n"
             "    \"outputIndex\"  (number) The output index\n"
             "    \"script\"  (strin) The script hex encoded\n"
             "    \"satoshis\"  (number) The number of satoshis of the output\n"
             "  }\n"
             "]\n"
             "\nExamples:\n"
             + HelpExampleCli("getaddressutxos", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
             + HelpExampleRpc("getaddressutxos", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
             );
 
     CAmount requiredAmount = 0;
     if (!request.params[1].isNull()) {
         requiredAmount = AmountFromValue(request.params[1]);
     }
 
     bool includeChainInfo = false;
     if (!request.params[2].isNull()) {
         includeChainInfo = request.params[2].get_bool();
     }
 
     std::vector<std::pair<uint160, int> > addresses;
 
     if (!getAddressesFromParams(request.params, addresses)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > unspentOutputs;
 
     for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
         if (!GetAddressUnspent((*it).first, (*it).second, unspentOutputs)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
         }
     }
 
     std::sort(unspentOutputs.begin(), unspentOutputs.end(), heightSort);
 
     UniValue utxos(UniValue::VARR);
     CAmount total = 0;
     for (std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> >::const_iterator it=unspentOutputs.begin(); it!=unspentOutputs.end(); it++) {
         if (requiredAmount > 0 && total >= requiredAmount) {
             break;
         }
 
         UniValue output(UniValue::VOBJ);
         std::string address;
         if (!getAddressFromIndex(it->first.type, it->first.hashBytes, address)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
         }
 
         output.pushKV("address", address);
         output.pushKV("txid", it->first.txhash.GetHex());
         output.pushKV("outputIndex", (int)it->first.index);
         output.pushKV("script", HexStr(it->second.script.begin(), it->second.script.end()));
         output.pushKV("satoshis", it->second.satoshis);
         output.pushKV("height", it->second.blockHeight);
 
         utxos.push_back(output);
 
         total += it->second.satoshis;
     }
 
     if (includeChainInfo) {
         UniValue result(UniValue::VOBJ);
         result.pushKV("utxos", utxos);
 
         LOCK(cs_main);
         result.pushKV("hash", chainActive.Tip()->GetBlockHash().GetHex());
         result.pushKV("height", (int)chainActive.Height());
         return result;
     } else {
         return utxos;
     }
 }
 
 UniValue getaddressmempool(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() != 1)
         throw std::runtime_error(
             "getaddressmempool\n"
             "\nReturns all mempool deltas for an address (requires addressindex to be enabled).\n"
             "\nArguments:\n"
             "{\n"
             "  \"addresses\"\n"
             "    [\n"
             "      \"address\"  (string) The base58check encoded address\n"
             "      ,...\n"
             "    ]\n"
             "}\n"
             "\nResult:\n"
             "[\n"
             "  {\n"
             "    \"address\"  (string) The base58check encoded address\n"
             "    \"txid\"  (string) The related txid\n"
             "    \"index\"  (number) The related input or output index\n"
             "    \"satoshis\"  (number) The difference of satoshis\n"
             "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
             "    \"prevtxid\"  (string) The previous txid (if spending)\n"
             "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
             "  }\n"
             "]\n"
             "\nExamples:\n"
             + HelpExampleCli("getaddressmempool", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
             + HelpExampleRpc("getaddressmempool", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
         );
 
     std::vector<std::pair<uint160, int> > addresses;
 
     if (!getAddressesFromParams(request.params, addresses)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> > indexes;
 
     if (!mempool.getAddressIndex(addresses, indexes)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
     }
 
     std::sort(indexes.begin(), indexes.end(), timestampSort);
 
     UniValue result(UniValue::VARR);
 
     for (std::vector<std::pair<CMempoolAddressDeltaKey, CMempoolAddressDelta> >::iterator it = indexes.begin();
          it != indexes.end(); it++) {
 
         std::string address;
         if (!getAddressFromIndex(it->first.type, it->first.addressBytes, address)) {
             throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unknown address type");
         }
 
         UniValue delta(UniValue::VOBJ);
         delta.pushKV("address", address);
         delta.pushKV("txid", it->first.txhash.GetHex());
         delta.pushKV("index", (int)it->first.index);
         delta.pushKV("satoshis", it->second.amount);
         delta.pushKV("timestamp", it->second.time);
         if (it->second.amount < 0) {
             delta.pushKV("prevtxid", it->second.prevhash.GetHex());
             delta.pushKV("prevout", (int)it->second.prevout);
         }
         result.push_back(delta);
     }
 
     return result;
 }
 
 UniValue getblockhashes(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() < 2)
         throw std::runtime_error(
             "getblockhashes timestamp\n"
             "\nReturns array of hashes of blocks within the timestamp range provided.\n"
             "\nArguments:\n"
             "1. high         (numeric, required) The newer block timestamp\n"
             "2. low          (numeric, required) The older block timestamp\n"
             "3. options      (string, required) A json object\n"
             "    {\n"
             "      \"noOrphans\":true   (boolean) will only include blocks on the main chain\n"
             "      \"logicalTimes\":true   (boolean) will include logical timestamps with hashes\n"
             "    }\n"
             "\nResult:\n"
             "[\n"
             "  \"hash\"         (string) The block hash\n"
             "]\n"
             "[\n"
             "  {\n"
             "    \"blockhash\": (string) The block hash\n"
             "    \"logicalts\": (numeric) The logical timestamp\n"
             "  }\n"
             "]\n"
             "\nExamples:\n"
             + HelpExampleCli("getblockhashes", "1231614698 1231024505")
             + HelpExampleRpc("getblockhashes", "1231614698, 1231024505")
             + HelpExampleCli("getblockhashes", "1231614698 1231024505 '{\"noOrphans\":false, \"logicalTimes\":true}'")
             );
 
     unsigned int high = request.params[0].get_int();
     unsigned int low = request.params[1].get_int();
     bool fActiveOnly = false;
     bool fLogicalTS = false;
 
     if (request.params.size() > 2) {
         if (request.params[2].isObject()) {
             UniValue noOrphans = find_value(request.params[2].get_obj(), "noOrphans");
             UniValue returnLogical = find_value(request.params[2].get_obj(), "logicalTimes");
 
             if (noOrphans.isBool())
                 fActiveOnly = noOrphans.get_bool();
 
             if (returnLogical.isBool())
                 fLogicalTS = returnLogical.get_bool();
         }
     }
 
     std::vector<std::pair<uint256, unsigned int> > blockHashes;
 
     if (fActiveOnly)
         LOCK(cs_main);
 
     if (!GetTimestampIndex(high, low, fActiveOnly, blockHashes)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for block hashes");
     }
 
     UniValue result(UniValue::VARR);
 
     for (std::vector<std::pair<uint256, unsigned int> >::const_iterator it=blockHashes.begin(); it!=blockHashes.end(); it++) {
         if (fLogicalTS) {
             UniValue item(UniValue::VOBJ);
             item.pushKV("blockhash", it->first.GetHex());
             item.pushKV("logicalts", (int)it->second);
             result.push_back(item);
         } else {
             result.push_back(it->first.GetHex());
         }
     }
 
     return result;
 }
 
 UniValue getspentinfo(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() != 1 || !request.params[0].isObject())
         throw std::runtime_error(
             "getspentinfo\n"
             "\nReturns the txid and index where an output is spent.\n"
             "\nArguments:\n"
             "{\n"
             "  \"txid\" (string) The hex string of the txid\n"
             "  \"index\" (number) The start block height\n"
             "}\n"
             "\nResult:\n"
             "{\n"
             "  \"txid\"  (string) The transaction id\n"
             "  \"index\"  (number) The spending input index\n"
             "  ,...\n"
             "}\n"
             "\nExamples:\n"
             + HelpExampleCli("getspentinfo", "'{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}'")
             + HelpExampleRpc("getspentinfo", "{\"txid\": \"0437cd7f8525ceed2324359c2d0ba26006d92d856a9c20fa0241106ee5a597c9\", \"index\": 0}")
         );
 
     UniValue txidValue = find_value(request.params[0].get_obj(), "txid");
     UniValue indexValue = find_value(request.params[0].get_obj(), "index");
 
     if (!txidValue.isStr() || !indexValue.isNum()) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid txid or index");
     }
 
     uint256 txid = ParseHashV(txidValue, "txid");
     int outputIndex = indexValue.get_int();
 
     CSpentIndexKey key(txid, outputIndex);
     CSpentIndexValue value;
 
     if (!GetSpentIndex(key, value)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unable to get spent info");
     }
 
     UniValue obj(UniValue::VOBJ);
     obj.pushKV("txid", value.txid.GetHex());
     obj.pushKV("index", (int)value.inputIndex);
     obj.pushKV("height", value.blockHeight);
 
     return obj;
 }
 
 UniValue getaddresstxids(const JSONRPCRequest& request)
 {
     if (request.fHelp || request.params.size() != 1)
         throw std::runtime_error(
             "getaddresstxids\n"
             "\nReturns the txids for an address(es) (requires addressindex to be enabled).\n"
             "\nArguments:\n"
             "{\n"
             "  \"addresses\"\n"
             "    [\n"
             "      \"address\"  (string) The base58check encoded address\n"
             "      ,...\n"
             "    ]\n"
             "  \"start\" (number) The start block height\n"
             "  \"end\" (number) The end block height\n"
             "}\n"
             "\nResult:\n"
             "[\n"
             "  \"transactionid\"  (string) The transaction id\n"
             "  ,...\n"
             "]\n"
             "\nExamples:\n"
             + HelpExampleCli("getaddresstxids", "'{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}'")
             + HelpExampleRpc("getaddresstxids", "{\"addresses\": [\"12c6DSiU4Rq3P4ZxziKxzrL5LmMBrzjrJX\"]}")
         );
 
     std::vector<std::pair<uint160, int> > addresses;
 
     if (!getAddressesFromParams(request.params, addresses)) {
         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
     }
 
     int start = 0;
     int end = 0;
     // if (request.params[0].isObject()) {
     //     UniValue startValue = find_value(request.params[0].get_obj(), "start");
     //     UniValue endValue = find_value(request.params[0].get_obj(), "end");
     //     if (startValue.isNum() && endValue.isNum()) {
     //         start = startValue.get_int();
     //         end = endValue.get_int();
     //     }
     // }
 
     std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
 
     for (std::vector<std::pair<uint160, int> >::iterator it = addresses.begin(); it != addresses.end(); it++) {
         if (start > 0 && end > 0) {
             if (!GetAddressIndex((*it).first, (*it).second, addressIndex, start, end)) {
                 throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
             }
         } else {
             if (!GetAddressIndex((*it).first, (*it).second, addressIndex)) {
                 throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No information available for address");
             }
         }
     }
 
     std::set<std::pair<int, std::string> > txids;
     UniValue result(UniValue::VARR);
 
     for (std::vector<std::pair<CAddressIndexKey, CAmount> >::const_iterator it=addressIndex.begin(); it!=addressIndex.end(); it++) {
         int height = it->first.blockHeight;
         std::string txid = it->first.txhash.GetHex();
 
         if (addresses.size() > 1) {
             txids.insert(std::make_pair(height, txid));
         } else {
             if (txids.insert(std::make_pair(height, txid)).second) {
                 result.push_back(txid);
             }
         }
     }
 
     if (addresses.size() > 1) {
         for (std::set<std::pair<int, std::string> >::const_iterator it=txids.begin(); it!=txids.end(); it++) {
             result.push_back(it->second);
         }
     }

    return result;
}

UniValue createmultisig(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() < 2 || request.params.size() > 2)
    {
        string msg = "createmultisig nrequired [\"key\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "It returns a json object with the address and redeemScript.\n"

            "\nArguments:\n"
            "1. nrequired      (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keys\"       (string, required) A json array of keys which are shibacoin addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"key\"    (string) shibacoin address or hex-encoded public key\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "{\n"
            "  \"address\":\"multisigaddress\",  (string) The value of the new multisig address.\n"
            "  \"redeemScript\":\"script\"       (string) The string value of the hex-encoded redemption script.\n"
            "}\n"

            "\nExamples:\n"
            "\nCreate a multisig address from 2 addresses\n"
            + HelpExampleCli("createmultisig", "2 \"[\\\"PYYrKuRMkcSRovzPngiKLJ3UYFa8erQ5Ku\\\",\\\"Ph28hNGW8zc87ae38NfwCb2kYs8etcoQ5R\\\"]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("createmultisig", "2, \"[\\\"PYYrKuRMkcSRovzPngiKLJ3UYFa8erQ5Ku\\\",\\\"Ph28hNGW8zc87ae38NfwCb2kYs8etcoQ5R\\\"]\"")
        ;
        throw runtime_error(msg);
    }

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(request.params);
    CScriptID innerID(inner);
    CBitcoinAddress address(innerID);

    UniValue result(UniValue::VOBJ);
    result.pushKV("address", address.ToString());
    result.pushKV("redeemScript", HexStr(inner.begin(), inner.end()));

    return result;
}

UniValue verifymessage(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 3)
        throw runtime_error(
            "verifymessage \"address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"address\"         (string, required) The shibacoin address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"PhmtFeTAFUzmRi9bhJhRc6mNsqt2pjCYUD\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"PhmtFeTAFUzmRi9bhJhRc6mNsqt2pjCYUD\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"PhmtFeTAFUzmRi9bhJhRc6mNsqt2pjCYUD\", \"signature\", \"my message\"")
        );

    LOCK(cs_main);

    string strAddress  = request.params[0].get_str();
    string strSign     = request.params[1].get_str();
    string strMessage  = request.params[2].get_str();

    CBitcoinAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetID() == keyID);
}

UniValue signmessagewithprivkey(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 2)
        throw runtime_error(
            "signmessagewithprivkey \"privkey\" \"message\"\n"
            "\nSign a message with the private key of an address\n"
            "\nArguments:\n"
            "1. \"privkey\"         (string, required) The private key to sign the message with.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples:\n"
            "\nCreate the signature\n"
            + HelpExampleCli("signmessagewithprivkey", "\"privkey\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"PhmtFeTAFUzmRi9bhJhRc6mNsqt2pjCYUD\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("signmessagewithprivkey", "\"privkey\", \"my message\"")
        );

    string strPrivkey = request.params[0].get_str();
    string strMessage = request.params[1].get_str();

    CBitcoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strPrivkey);
    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key");
    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

UniValue setmocktime(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 1)
        throw runtime_error(
            "setmocktime timestamp\n"
            "\nSet the local time to given timestamp (-regtest only)\n"
            "\nArguments:\n"
            "1. timestamp  (integer, required) Unix seconds-since-epoch timestamp\n"
            "   Pass 0 to go back to using the system time."
        );

    if (!Params().MineBlocksOnDemand())
        throw runtime_error("setmocktime for regression testing (-regtest mode) only");

    // For now, don't change mocktime if we're in the middle of validation, as
    // this could have an effect on mempool time-based eviction, as well as
    // IsCurrentForFeeEstimation() and IsInitialBlockDownload().
    // TODO: figure out the right way to synchronize around mocktime, and
    // ensure all callsites of GetTime() are accessing this safely.
    LOCK(cs_main);

    RPCTypeCheck(request.params, boost::assign::list_of(UniValue::VNUM));
    SetMockTime(request.params[0].get_int64());

    return NullUniValue;
}

UniValue getmocktime(const JSONRPCRequest& request)
{
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getmocktime\n"
            "\nGet the current mocktime (-regtest only)\n"
            "Note: this is an asynchronous read, and does not take into account any\n"
            "setmocktime calls that are pending."
            "\nResult:\n"
            "\"timestamp\"          (int64) The current mocktime (0 = no mocktime is set)\n"
            "\nExamples:\n"
            + HelpExampleCli("getmocktime", "")
            + HelpExampleRpc("getmocktime", "")
        );

    UniValue result(GetMockTime());
    return result;
}

static UniValue RPCLockedMemoryInfo()
{
    LockedPool::Stats stats = LockedPoolManager::Instance().stats();
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("used", uint64_t(stats.used));
    obj.pushKV("free", uint64_t(stats.free));
    obj.pushKV("total", uint64_t(stats.total));
    obj.pushKV("locked", uint64_t(stats.locked));
    obj.pushKV("chunks_used", uint64_t(stats.chunks_used));
    obj.pushKV("chunks_free", uint64_t(stats.chunks_free));
    return obj;
}

UniValue getmemoryinfo(const JSONRPCRequest& request)
{
    /* Please, avoid using the word "pool" here in the RPC interface or help,
     * as users will undoubtedly confuse it with the other "memory pool"
     */
    if (request.fHelp || request.params.size() != 0)
        throw runtime_error(
            "getmemoryinfo\n"
            "Returns an object containing information about memory usage.\n"
            "\nResult:\n"
            "{\n"
            "  \"locked\": {               (json object) Information about locked memory manager\n"
            "    \"used\": xxxxx,          (numeric) Number of bytes used\n"
            "    \"free\": xxxxx,          (numeric) Number of bytes available in current arenas\n"
            "    \"total\": xxxxxxx,       (numeric) Total number of bytes managed\n"
            "    \"locked\": xxxxxx,       (numeric) Amount of bytes that succeeded locking. If this number is smaller than total, locking pages failed at some point and key data could be swapped to disk.\n"
            "    \"chunks_used\": xxxxx,   (numeric) Number allocated chunks\n"
            "    \"chunks_free\": xxxxx,   (numeric) Number unused chunks\n"
            "  }\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getmemoryinfo", "")
            + HelpExampleRpc("getmemoryinfo", "")
        );
    UniValue obj(UniValue::VOBJ);
    obj.pushKV("locked", RPCLockedMemoryInfo());
    return obj;
}

UniValue echo(const JSONRPCRequest& request)
{
    if (request.fHelp)
        throw runtime_error(
            "echo|echojson \"message\" ...\n"
            "\nSimply echo back the input arguments. This command is for testing.\n"
            "\nThe difference between echo and echojson is that echojson has argument conversion enabled in the client-side table in"
            "shibacoin-cli and the GUI. There is no server-side difference."
        );

    return request.params;
}

static const CRPCCommand commands[] =
{ //  category              name                      actor (function)         okSafeMode
  //  --------------------- ------------------------  -----------------------  ----------
    { "control",            "getinfo",                &getinfo,                true,  {} }, /* uses wallet if enabled */
    { "control",            "getmemoryinfo",          &getmemoryinfo,          true,  {} },
    { "util",               "validateaddress",        &validateaddress,        true,  {"address"} }, /* uses wallet if enabled */
    { "util",               "createmultisig",         &createmultisig,         true,  {"nrequired","keys"} },
    { "util",               "verifymessage",          &verifymessage,          true,  {"address","signature","message"} },
    { "util",               "signmessagewithprivkey", &signmessagewithprivkey, true,  {"privkey","message"} },
    { "util",               "getaddresstxids",        &getaddresstxids,        true,  {"address"} },
    { "util",               "getaddressdeltas",       &getaddressdeltas,       true,  {"address"} },
    { "util",               "getaddressbalance",      &getaddressbalance,      true,  {"address"} },
    { "util",               "getaddressutxos",        &getaddressutxos,        true,  {"address"} },
    { "util",               "getaddressmempool",      &getaddressmempool,      true,  {"address"} },
    { "util",               "getblockhashes",         &getblockhashes,         true,  {"high","low","options"} },
    { "util",               "getspentinfo",           &getspentinfo,           true,  {"argument"} },

    /* Not shown in help */
    { "hidden",             "setmocktime",            &setmocktime,            true,  {"timestamp"}},
    { "hidden",             "getmocktime",            &getmocktime,            true,  {}},
    { "hidden",             "echo",                   &echo,                   true,  {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
    { "hidden",             "echojson",               &echo,                  true,  {"arg0","arg1","arg2","arg3","arg4","arg5","arg6","arg7","arg8","arg9"}},
};

void RegisterMiscRPCCommands(CRPCTable &t)
{
    for (unsigned int vcidx = 0; vcidx < ARRAYLEN(commands); vcidx++)
        t.appendCommand(commands[vcidx].name, &commands[vcidx]);
}
