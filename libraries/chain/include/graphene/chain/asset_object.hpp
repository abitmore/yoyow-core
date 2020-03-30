/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once
#include <graphene/chain/protocol/asset_ops.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/db/generic_index.hpp>

/**
 * @defgroup prediction_market Prediction Market
 *
 * A prediction market is a specialized BitAsset such that total debt and total collateral are always equal amounts
 * (although asset IDs differ). No margin calls or force settlements may be performed on a prediction market asset. A
 * prediction market is globally settled by the issuer after the event being predicted resolves, thus a prediction
 * market must always have the @ref global_settle permission enabled. The maximum price for global settlement or short
 * sale of a prediction market asset is 1-to-1.
 */

namespace graphene { namespace chain {
   class account_object;
   class database;
   using namespace graphene::db;

   /**
    *  @brief tracks the asset information that changes frequently
    *  @ingroup object
    *  @ingroup implementation
    *
    *  Because the asset_object is very large it doesn't make sense to save an undo state
    *  for all of the parameters that never change.   This object factors out the parameters
    *  of an asset that change in almost every transaction that involves the asset.
    *
    *  This object exists as an implementation detail and its ID should never be referenced by
    *  a blockchain operation.
    */
   class asset_dynamic_data_object : public abstract_object<asset_dynamic_data_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_asset_dynamic_data_type;

         /// asset ID (AID)
         asset_aid_type asset_id = 0;
         /// The number of shares currently in existence
         share_type current_supply;
         //share_type confidential_supply; ///< total asset held in confidential balances
         share_type accumulated_fees; ///< fees accumulate to be paid out over time
         //share_type fee_pool;         ///< in core asset
   };

   /**
    *  @brief tracks the parameters of an asset
    *  @ingroup object
    *
    *  All assets have a globally unique symbol name that controls how they are traded and an issuer who
    *  has authority over the parameters of the asset.
    */
   class asset_object : public graphene::db::abstract_object<asset_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id  = asset_object_type;

         /// @return true if this is a market-issued asset; false otherwise.
         //bool is_market_issued()const { return bitasset_data_id.valid(); }
         /// @return true if users may request force-settlement of this market-issued asset; false otherwise
         //bool can_force_settle()const { return !(options.flags & disable_force_settle); }
         /// @return true if the issuer of this market-issued asset may globally settle the asset; false otherwise
         //bool can_global_settle()const { return options.issuer_permissions & global_settle; }

         /// @return true if this asset is using whitelist or blacklist
         bool enabled_whitelist()const { return options.flags & white_list; }
         /// @return true if the issuer can create new supply
         bool can_issue_asset()const { return options.flags & issue_asset; }
         /// @return true if the issuer can change max supply
         bool can_change_max_supply()const { return options.flags & change_max_supply; }

         /// @return true if this asset charges a fee for the issuer on market operations; false otherwise
         bool charges_market_fees()const { return options.flags & charge_market_fee; }
         /// @return true if this asset may only be transferred to/from the issuer or market orders
         bool is_transfer_restricted()const { return options.flags & transfer_restricted; }
         /// @return true if the issuer can transfer asset back to himself
         bool can_override()const { return options.flags & override_authority; }
         //bool allow_confidential()const { return !(options.flags & asset_issuer_permission_flags::disable_confidential); }

         /// Helper function to get an asset object with the given amount in this asset's type
         asset amount(share_type a)const { return asset(a, asset_id); }
         /// Convert a string amount (i.e. "123.45") to an asset object with this asset's type
         /// The string may have a decimal and/or a negative sign.
         asset amount_from_string(string amount_string)const;
         /// Convert an asset to a textual representation, i.e. "123.45"
         string amount_to_string(share_type amount)const;
         /// Convert an asset to a textual representation, i.e. "123.45"
         string amount_to_string(const asset& amount)const
         { FC_ASSERT(amount.asset_id == id.instance()); return amount_to_string(amount.amount); }
         /// Convert an asset to a textual representation with symbol, i.e. "123.45 USD"
         string amount_to_pretty_string(share_type amount)const
         { return amount_to_string(amount) + " " + symbol; }
         /// Convert an asset to a textual representation with symbol, i.e. "123.45 USD"
         string amount_to_pretty_string(const asset &amount)const
         { FC_ASSERT(amount.asset_id == id.instance()); return amount_to_pretty_string(amount.amount); }

         /// asset ID (AID)
         asset_aid_type asset_id = 0;
         /// Ticker symbol for this asset, i.e. "YOYO"
         string symbol;
         /// Maximum number of digits after the decimal point (must be <= 12)
         uint8_t precision = 0;
         /// ID of the account which issued this asset.
         account_uid_type issuer;

         asset_options options;


         /// Current supply, fee pool, and collected fees are stored in a separate object as they change frequently.
         asset_dynamic_data_id_type  dynamic_asset_data_id;
         /// Extra data associated with BitAssets. This field is non-null if and only if is_market_issued() returns true
         //optional<asset_bitasset_data_id_type> bitasset_data_id;

         asset_id_type get_id()const { return id; }
         asset_aid_type get_asset_id()const { return asset_id; }

         void validate()const
         {
         }

         template<class DB>
         const asset_dynamic_data_object& dynamic_data(const DB& db)const
         { return db.get(dynamic_asset_data_id); }

         /**
          *  The total amount of an asset that is reserved for future issuance. 
          */
         template<class DB>
         share_type reserved( const DB& db )const
         { return std::max( share_type(0), options.max_supply - dynamic_data(db).current_supply ); }
   };

   struct by_aid;
   struct by_symbol;
   struct by_issuer;
   typedef multi_index_container<
      asset_object,
      indexed_by<
         ordered_unique< tag<by_id>, member< object, object_id_type, &object::id > >,
         ordered_unique< tag<by_aid>, member< asset_object, asset_aid_type, &asset_object::asset_id > >,
         ordered_unique< tag<by_symbol>, member<asset_object, string, &asset_object::symbol> >,
         ordered_non_unique< tag<by_issuer>, member<asset_object, account_uid_type, &asset_object::issuer > >
      >
   > asset_object_multi_index_type;
   typedef generic_index<asset_object, asset_object_multi_index_type> asset_index;

} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::asset_dynamic_data_object, (graphene::db::object),
   (asset_id)(current_supply)(accumulated_fees))

FC_REFLECT_DERIVED( graphene::chain::asset_object, (graphene::db::object),
                    (asset_id)
                    (symbol)
                    (precision)
                    (issuer)
                    (options)
                    (dynamic_asset_data_id)
                  )
