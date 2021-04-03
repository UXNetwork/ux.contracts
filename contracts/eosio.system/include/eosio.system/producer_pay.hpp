#include <algorithm>

namespace eosiosystem {

   using eosio::time_point_sec;
   using eosio::name;
   using eosio::asset;

   struct [[eosio::table, eosio::contract("eosio.system")]] producer_pay {
      name             owner;
      asset            balance;
      time_point_sec   last_claim_time;

      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_pay, (owner)(balance)(last_claim_time) )
   };

   typedef eosio::multi_index< "prodpay"_n, producer_pay >  producer_pay_table;
}