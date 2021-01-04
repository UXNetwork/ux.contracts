#pragma once

#define UX_CORE_SYM_NAME "UTX"
#define RAM_CORE_SYM_NAME "UTXRAM"

#define UX_CORE_SYM_PRECISION 4
#define RAM_CORE_SYM_PRECISION 4

#define _STRINGIZE1(x) #x
#define _STRINGIZE2(x) _STRINGIZE1(x)

#define UX_CORE_SYM_STR ( _STRINGIZE2(UX_CORE_SYM_PRECISION) "," UX_CORE_SYM_NAME )
#define UX_CORE_SYM  ( ::eosio::chain::string_to_symbol_c( UX_CORE_SYM_PRECISION, UX_CORE_SYM_NAME ) )

#define RAM_CORE_SYM_STR ( _STRINGIZE2(RAM_CORE_SYM_PRECISION) "," RAM_CORE_SYM_NAME )
#define RAM_CORE_SYM  ( ::eosio::chain::string_to_symbol_c( RAM_CORE_SYM_PRECISION, RAM_CORE_SYM_NAME ) )

struct ux_core_sym {
   static inline eosio::chain::asset from_string(const std::string& s) {
     return eosio::chain::asset::from_string(s + " " UX_CORE_SYM_NAME);
   }
};

struct ram_core_sym {
   static inline eosio::chain::asset from_string(const std::string& s) {
     return eosio::chain::asset::from_string(s + " " RAM_CORE_SYM_NAME);
   }
};


