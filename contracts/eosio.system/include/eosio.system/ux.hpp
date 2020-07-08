#include <algorithm>

namespace eosiosystem {

   using eosio::time_point_sec;
   using eosio::time_point;
   using eosio::name;

   struct [[eosio::table, eosio::contract("eosio.system")]] producer_pay {
      name             owner;
      uint64_t         earned_pay;
      uint64_t         last_claim_time = 0;

      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_pay, (owner)(earned_pay)(last_claim_time) )
   };

   struct [[eosio::table("uxglobal"), eosio::contract("eosio.system")]] ux_global_state {
      time_point_sec        last_inflation_print = time_point_sec();
      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( ux_global_state, (last_inflation_print) )
   };

   typedef eosio::multi_index< "prodpay"_n, producer_pay >  producer_pay_table;
   typedef eosio::singleton< "uxglobal"_n, ux_global_state >   uxglobal_state_singleton;
}