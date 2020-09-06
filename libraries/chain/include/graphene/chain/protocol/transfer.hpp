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
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

namespace graphene { namespace chain {

   /**
    * @ingroup operations
    *
    * @brief Transfers an amount of one asset from one account to another
    *
    *  Fees are paid by the "from" account
    *
    *  @pre amount.amount > 0
    *  @pre fee.amount >= 0
    *  @pre from != to
    *  @post from account's balance will be reduced by fee and amount
    *  @post to account's balance will be increased by amount
    *  @return n/a
    */
   struct transfer_operation : public base_operation
   {
      struct ext
      {
         optional< asset > from_balance;
         optional< asset > from_prepaid;
         optional< asset > to_balance;
         optional< asset > to_prepaid;

         optional<account_uid_type>   sign_platform;    // sign by platform account
      };

      struct fee_parameters_type {
         uint64_t fee              = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type          fee;
      /// Account to transfer asset from
      account_uid_type  from;
      /// Account to transfer asset to
      account_uid_type  to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset            amount;

      /// User provided data encrypted to the memo key of the "to" account
      optional< memo_data > memo;

      optional< extension< ext > > extensions;

      account_uid_type fee_payer_uid()const { return from; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
      bool some_from_balance() const
      {
         return ( !extensions.valid() ||
                  ( extensions->value.from_balance.valid() &&
                    extensions->value.from_balance->amount > 0 ) );
      }
      void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
         // need active authority if transfer from balance
         if( some_from_balance() )
            a.insert( from );
      }
      void get_required_secondary_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
      {
         // need secondary authority if not transfer from balance
         // note: this means that owner authority is not enough nor needed
         if( !some_from_balance() )
            a.insert( from );
      }
   };
   
   /**
    * @ingroup operations
    *
    * @brief Transfers an amount of one asset from one account to another, with plain memo
    *
    *  Fees are paid by the "from" account
    *
    *  @pre amount.amount > 0
    *  @pre fee.amount >= 0
    *  @pre from != to
    *  @post from account's balance will be reduced by fee and amount
    *  @post to account's balance will be increased by amount
    *  @return n/a
    */
   struct inline_transfer_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = GRAPHENE_BLOCKCHAIN_PRECISION/10;
         uint32_t price_per_kbyte  = GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type            fee;
      account_uid_type  from;
      account_uid_type  to;
      asset            amount;

      string           memo;
      extensions_type  extensions;

      account_uid_type fee_payer_uid()const { return from; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
   };

   /**
    *  @class override_transfer_operation
    *  @brief Allows the issuer of an asset to transfer an asset from any account to any account if they have override_authority
    *  @ingroup operations
    *
    *  @pre amount.asset_id->issuer == issuer
    *  @pre issuer != from  because this is pointless, use a normal transfer operation
    */
   struct override_transfer_operation : public base_operation
   {
      struct fee_parameters_type {
         uint64_t fee              = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
         uint32_t price_per_kbyte  = GRAPHENE_BLOCKCHAIN_PRECISION; /// only required for large memos.
         uint64_t min_real_fee     = 0;
         uint16_t min_rf_percent   = 0;
         extensions_type   extensions;
      };

      fee_type         fee;
      account_uid_type issuer;
      /// Account to transfer asset from
      account_uid_type from;
      /// Account to transfer asset to
      account_uid_type to;
      /// The amount of asset to transfer from @ref from to @ref to
      asset amount;

      /// User provided data encrypted to the memo key of the "to" account
      optional<memo_data> memo;
      extensions_type   extensions;

      account_uid_type fee_payer_uid()const { return issuer; }
      void            validate()const;
      share_type      calculate_fee(const fee_parameters_type& k)const;
	  void get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const      
	  {
	  	if(enabled_hardfork)
			a.insert(issuer);
	  }
   };

}} // graphene::chain



FC_REFLECT_TYPENAME( graphene::chain::extension<graphene::chain::transfer_operation::ext>)

FC_REFLECT( graphene::chain::transfer_operation::fee_parameters_type,
            (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )
FC_REFLECT( graphene::chain::override_transfer_operation::fee_parameters_type,
            (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions) )

            FC_REFLECT(graphene::chain::transfer_operation::ext, (from_balance)(from_prepaid)(to_balance)(to_prepaid)(sign_platform))
FC_REFLECT( graphene::chain::transfer_operation, (fee)(from)(to)(amount)(memo)(extensions) )
FC_REFLECT( graphene::chain::override_transfer_operation, (fee)(issuer)(from)(to)(amount)(memo)(extensions) )
FC_REFLECT( graphene::chain::inline_transfer_operation::fee_parameters_type,  (fee)(price_per_kbyte)(min_real_fee)(min_rf_percent)(extensions)  )
FC_REFLECT( graphene::chain::inline_transfer_operation, (fee)(from)(to)(amount)(memo)(extensions) )