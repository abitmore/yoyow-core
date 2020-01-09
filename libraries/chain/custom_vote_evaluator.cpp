/*
 * Copyright (c) 2018, YOYOW Foundation PTE. LTD. and contributors.
 */
#pragma once
#include <graphene/chain/custom_vote_evaluator.hpp>
#include <graphene/chain/custom_vote_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/is_authorized_asset.hpp>

namespace graphene { namespace chain {

void_result custom_vote_create_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only create custom vote after HARDFORK_0_4_TIME");

      d.get_account_by_uid(op.custom_vote_creator);//check create account exist
      account_stats = &d.get_account_statistics_by_uid(op.custom_vote_creator);
      FC_ASSERT((account_stats->last_custom_vote_sequence + 1) == op.vote_vid,"vote_vid ${vid} is invalid.",("vid", op.vote_vid)); 

      d.get_asset_by_aid(op.vote_asset_id); //check asset exist

      const auto& params = d.get_global_properties().parameters.get_award_params();
      auto range_end_time = d.head_block_time() + params.custom_vote_effective_time;
      FC_ASSERT(op.vote_expired_time > d.head_block_time() && op.vote_expired_time < range_end_time,
         "vote expired time should in range ${start}--${end}", ("start", d.head_block_time())("end", range_end_time));

      return void_result();

   }FC_CAPTURE_AND_RETHROW((op))
}

object_id_type custom_vote_create_evaluator::do_apply(const operation_type& op)
{
   try {
      database& d = db();
      const auto& custom_vote_obj = d.create<custom_vote_object>([&](custom_vote_object& obj)
      {
         obj.custom_vote_creator = op.custom_vote_creator;
         obj.vote_vid = op.vote_vid;
         obj.title = op.title;
         obj.description = op.description;
         obj.vote_expired_time = op.vote_expired_time;
         obj.vote_asset_id = op.vote_asset_id;
         obj.required_asset_amount = op.required_asset_amount;
         obj.minimum_selected_items = op.minimum_selected_items;
         obj.maximum_selected_items = op.maximum_selected_items;

         obj.vote_result.resize(op.options.size());
         obj.options = op.options;
      });

      d.modify(*account_stats, [&](_account_statistics_object& s) {
         s.last_custom_vote_sequence += 1;
      });

      return custom_vote_obj.id;
   } FC_CAPTURE_AND_RETHROW((op))
}

void_result custom_vote_cast_evaluator::do_evaluate(const operation_type& op)
{
   try {
      const database& d = db();
      FC_ASSERT(d.head_block_time() >= HARDFORK_0_4_TIME, "Can only cast custom vote after HARDFORK_0_4_TIME");

      d.get_account_by_uid(op.voter);//check custom voter account exist
       
      auto custom_vote_obj = d.find_custom_vote_by_vid(op.custom_vote_creator, op.custom_vote_vid);
      FC_ASSERT(custom_vote_obj != nullptr, "custom vote ${vid} not found.", ("id", op.custom_vote_vid));
      FC_ASSERT(d.head_block_time() <= custom_vote_obj->vote_expired_time, "custom vote already overdue");
      FC_ASSERT(op.vote_result.size() >= custom_vote_obj->minimum_selected_items && op.vote_result.size() <= custom_vote_obj->maximum_selected_items, 
         "vote options num is not in range ${min} - ${max}.", ("min", custom_vote_obj->minimum_selected_items)("max", custom_vote_obj->maximum_selected_items));

      auto votes = d.get_account_statistics_by_uid(op.voter).get_votes_from_core_balance();
      FC_ASSERT(votes >= custom_vote_obj->required_asset_amount, "asset ${aid} balance less than required amount for vote ${amount}", 
         ("aid", custom_vote_obj->vote_asset_id)("amount", custom_vote_obj->required_asset_amount));

      auto last_index = *(op.vote_result.rbegin());
      FC_ASSERT(last_index < custom_vote_obj->options.size(), "option ${item} is not existent", ("item", last_index));
         
      return void_result();
   }FC_CAPTURE_AND_RETHROW((op))
}

void_result custom_vote_cast_evaluator::do_apply(const operation_type& op)
{
   try {
      return void_result();
   } FC_CAPTURE_AND_RETHROW((op))
}

} } // graphene::chain
