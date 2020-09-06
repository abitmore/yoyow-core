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
#include <graphene/chain/protocol/operations.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>

namespace graphene { namespace chain {

void proposal_create_operation::validate() const
{
   validate_op_fee( fee, "proposal create " );
   validate_account_uid( fee_paying_account, "proposal create " );
   FC_ASSERT( !proposed_ops.empty() );
   for( const auto& op : proposed_ops ) operation_validate( op.op );
}

share_type proposal_create_operation::calculate_fee(const fee_parameters_type& k) const
{
   return share_type( k.fee ) + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void proposal_update_operation::validate() const
{
   validate_op_fee( fee, "proposal update " );
   validate_account_uid( fee_paying_account, "proposal update " );
   FC_ASSERT(!(secondary_approvals_to_add.empty() && secondary_approvals_to_remove.empty() &&
               active_approvals_to_add.empty() && active_approvals_to_remove.empty() &&
               owner_approvals_to_add.empty() && owner_approvals_to_remove.empty() &&
               key_approvals_to_add.empty() && key_approvals_to_remove.empty()));
   for( auto a : secondary_approvals_to_add )
   {
      FC_ASSERT(secondary_approvals_to_remove.find(a) == secondary_approvals_to_remove.end(),
                "Cannot add and remove approval at the same time.");
   }
   for( auto a : active_approvals_to_add )
   {
      FC_ASSERT(active_approvals_to_remove.find(a) == active_approvals_to_remove.end(),
                "Cannot add and remove approval at the same time.");
   }
   for( auto a : owner_approvals_to_add )
   {
      FC_ASSERT(owner_approvals_to_remove.find(a) == owner_approvals_to_remove.end(),
                "Cannot add and remove approval at the same time.");
   }
   for( auto a : key_approvals_to_add )
   {
      FC_ASSERT(key_approvals_to_remove.find(a) == key_approvals_to_remove.end(),
                "Cannot add and remove approval at the same time.");
   }
}

void proposal_delete_operation::validate() const
{
   validate_op_fee( fee, "proposal delete " );
   validate_account_uid( fee_paying_account, "proposal delete " );
}

share_type proposal_update_operation::calculate_fee(const fee_parameters_type& k) const
{
   return share_type( k.fee ) + calculate_data_fee( fc::raw::pack_size(*this), k.price_per_kbyte );
}

void proposal_update_operation::get_required_authorities( vector<authority>& o )const
{
   authority auth;
   for( const auto& k : key_approvals_to_add )
      auth.key_auths[k] = 1;
   for( const auto& k : key_approvals_to_remove )
      auth.key_auths[k] = 1;
   auth.weight_threshold = auth.key_auths.size();

   if( auth.key_auths.size() > 0 )
      o.emplace_back( std::move(auth) );
}

void proposal_update_operation::get_required_secondary_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
{
   for( const auto& i : secondary_approvals_to_add )    a.insert(i);
   for( const auto& i : secondary_approvals_to_remove ) a.insert(i);
}

void proposal_update_operation::get_required_active_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
{
   for( const auto& i : active_approvals_to_add )    a.insert(i);
   for( const auto& i : active_approvals_to_remove ) a.insert(i);

   if(enabled_hardfork)
		a.insert(fee_paying_account);
}

void proposal_update_operation::get_required_owner_uid_authorities( flat_set<account_uid_type>& a,bool enabled_hardfork )const
{
   for( const auto& i : owner_approvals_to_add )    a.insert(i);
   for( const auto& i : owner_approvals_to_remove ) a.insert(i);
}

} } // graphene::chain
