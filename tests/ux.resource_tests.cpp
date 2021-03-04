#include <boost/test/unit_test.hpp>
#include <eosio/chain/contract_table_objects.hpp>
#include <eosio/chain/global_property_object.hpp>
#include <eosio/chain/resource_limits.hpp>
#include <eosio/chain/wast_to_wasm.hpp>
#include <fc/log/logger.hpp>
#include <eosio/chain/exceptions.hpp>
#include <Runtime/Runtime.h>

#include "ux.system_tester.hpp"

#include <time.h>
#include <ctime>

using namespace eosio_system;

BOOST_AUTO_TEST_SUITE(ux_system_tests)

BOOST_FIXTURE_TEST_CASE(resource_core, ux_system_tester)
try
{
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 2;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.0;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);

   fc::variant usage_data_vo_50 = json_from_file_or_string("./tests/usage_data/usage_data_50.json");
   struct oracle_data usage_data_50 = generate_all_data_hash(usage_data_vo_50, dataset_batch_size);

   fc::variant usage_data_vo_75 = json_from_file_or_string("./tests/usage_data/usage_data_75.json");
   struct oracle_data usage_data_75 = generate_all_data_hash(usage_data_vo_75, dataset_batch_size);

   // Test addactusg fails before settotalusg
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("inflation not yet transferred"),
                       addactusg(N(defproducera), 1, usage_data_50.usage_datasets[1], period_start));

   // Set totals for oracle 1
   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducera), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   produce_blocks(2);

   // Test inflation values - not issued yet
   BOOST_TEST(get_balance(N(eosio.bpay)).get_amount() == 0);
   BOOST_TEST(get_balance(N(eosio.upay)).get_amount() == 0);

   // Test addactusg fails before oracle threshold met
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("inflation not yet transferred"),
                       addactusg(N(defproducera), 1, usage_data_50.usage_datasets[1], period_start));

   // Test that duplicate total for oracle 1 fails
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("total already set"),
                       settotalusg(N(defproducera), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   // Set totals for oracle 2
   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerb), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   // Test inflation transferred

   BOOST_TEST(get_balance(N(eosio.bpay)).get_amount() == 4290175274);
   BOOST_TEST(get_balance(N(eosio.upay)).get_amount() == 2860662061);

   // Test Fail on wrong period start
   long period_start_sec_bad = getSecondsSinceEpochUTC("2020-10-01 01:00:00");
   time_point_sec period_start_bad = time_point_sec(period_start_sec_bad);
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("period_start does not match current period_start"),
                       addactusg(N(defproducera), 1, usage_data_50.usage_datasets[1], period_start_bad));

   for (int i = 0; i < usage_data_50.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(), addactusg(N(defproducera), i + 1, usage_data_50.usage_datasets[i], period_start));
   }
   produce_blocks(2);

   // addactusg should fail for producerc
   for (int i = 0; i < usage_data_50.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(wasm_assert_msg("usage totals not set"),
                          addactusg(N(defproducerc), i + 1, usage_data_50.usage_datasets[i], period_start));
   }

   for (int i = 0; i < usage_data_50.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(), addactusg(N(defproducerb), i + 1, usage_data_50.usage_datasets[i], period_start));
   }
   produce_blocks(2);

   // Theshold met ... verify inflation distributed
   BOOST_TEST(get_balance(N(eosio.bpay)).get_amount() == 4290175274);
   BOOST_TEST(get_balance(N(eosio.upay)).get_amount() == 2860662061);

   // Test fail on account overage
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("insufficient unallocated cpu"), addactusg(N(defproducera), 3, usage_data_50.usage_datasets[1], period_start));

   produce_blocks(2);

   // next day
   skipAhead(getSecondsSinceEpochUTC("2020-10-02 00:00:00"));
   BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducera)));

   period_start_sec = getSecondsSinceEpochUTC("2020-10-02 00:00:00");
   period_start = time_point_sec(period_start_sec);

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducera), usage_data_75.total_cpu_usage_us, usage_data_75.total_net_usage_words, usage_data_75.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerb), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   // Modal threshold not yet met
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("inflation not yet transferred"),
                       addactusg(N(defproducerb), 1, usage_data_50.usage_datasets[0], period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerc), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   for (int i = 0; i < usage_data_50.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerb), i + 1, usage_data_50.usage_datasets[i], period_start));
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerc), i + 1, usage_data_50.usage_datasets[i], period_start));
   }

   // next day
   // Test misbehaving oracle (defproducera)
   skipAhead(getSecondsSinceEpochUTC("2020-10-03 00:00:00"));
   BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerc)));

   period_start_sec = getSecondsSinceEpochUTC("2020-10-03 00:00:00");
   period_start = time_point_sec(period_start_sec);

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducera), usage_data_75.total_cpu_usage_us, usage_data_75.total_net_usage_words, usage_data_75.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerb), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerc), usage_data_75.total_cpu_usage_us, usage_data_75.total_net_usage_words, usage_data_75.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerd), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       addactusg(N(defproducera), 1, usage_data_75.usage_datasets[0], period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducere), usage_data_50.total_cpu_usage_us, usage_data_50.total_net_usage_words, usage_data_50.all_data_hash, period_start));

   for (int i = 1; i < usage_data_75.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducera), i + 1, usage_data_75.usage_datasets[i], period_start));
   }

   skipAhead(getSecondsSinceEpochUTC("2020-10-04 00:00:00"));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("full modal data not received"), nextperiod(N(defproducerc)));

   for (int i = 0; i < usage_data_50.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerb), i + 1, usage_data_50.usage_datasets[i], period_start));
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerc), i + 1, usage_data_50.usage_datasets[i], period_start));
   }

   BOOST_REQUIRE_EQUAL(wasm_assert_msg("full modal data not received"), nextperiod(N(defproducerc)));

   // Since 75% was the modal we need another 2 oracles to provide actusg for 75%
   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerf), usage_data_75.total_cpu_usage_us, usage_data_75.total_net_usage_words, usage_data_75.all_data_hash, period_start));

   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerg), usage_data_75.total_cpu_usage_us, usage_data_75.total_net_usage_words, usage_data_75.all_data_hash, period_start));

   for (int i = 0; i < usage_data_75.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerf), i + 1, usage_data_75.usage_datasets[i], period_start));
      BOOST_REQUIRE_EQUAL(success(),
                          addactusg(N(defproducerg), i + 1, usage_data_75.usage_datasets[i], period_start));
   }

   BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerf)));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(resource_nextperiod, ux_system_tester)
try
{
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 1;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.0;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);

   fc::variant usage_data_vo = json_from_file_or_string("./tests/usage_data/usage_data_50.json");
   struct oracle_data usage_data = generate_all_data_hash(usage_data_vo, dataset_batch_size);

   // Set totals for oracle 1
   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducera), usage_data.total_cpu_usage_us, usage_data.total_net_usage_words, usage_data.all_data_hash, period_start));

   produce_blocks(2);

   BOOST_REQUIRE_EQUAL(success(), addactusg(N(defproducera), 1, usage_data.usage_datasets[0], period_start));

   produce_blocks(2);

   skipAhead(getSecondsSinceEpochUTC("2020-10-03 23:59:59"));
   BOOST_REQUIRE_EQUAL(wasm_assert_msg("full modal data not received"), nextperiod(N(defproducera)));

   // Set totals for oracle 1
   BOOST_REQUIRE_EQUAL(success(),
                       settotalusg(N(defproducerb), usage_data.total_cpu_usage_us, usage_data.total_net_usage_words, usage_data.all_data_hash, period_start));

   produce_blocks(2);

   for (int i = 0; i < usage_data.usage_datasets.size(); i++)
   {
      BOOST_REQUIRE_EQUAL(success(), addactusg(N(defproducerb), i + 1, usage_data.usage_datasets[i], period_start));
   }
   BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerb)));
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(test_10_percent_inflation, ux_system_tester)
try
{
   uint32_t day = 0;
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 1;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.2947;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);

   asset pre_balance = get_token_supply();
   for (int i = 0; i < 365 * 3; i++)
   {
      dayCycle("./tests/usage_data/usage_data_10.json", period_start_sec, 5);
      period_start_sec += 86400;
      skipAhead(period_start_sec);
      day++;
      BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerb)));
      // .25 year inflation = 6.2%
      if (i == uint16_t(365 * .25))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 62);
      }
      // .50 year inflation = 5.1%
      if (i == uint16_t(365 * .50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 51);
      }
      // 1 year inflation = 3.6%
      if (i == uint16_t(365 * 1))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 36);
      }
      // 1.75 year inflation = 2.1%
      if (i == uint16_t(365 * 1.75))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 21);
      }
      // 2.50 year inflation = 1.2%
      if (i == uint16_t(365 * 2.50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 12);
      }
      // 3 year inflation = 0.88%
      if (i == uint16_t(365 * 3))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 88);
      }
   }
   asset post_balance = get_token_supply();
   cout << "10% utilization 3 year inflation: " << 100 * float((post_balance.get_amount() - pre_balance.get_amount())) / pre_balance.get_amount() << endl;
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(test_50_percent_inflation, ux_system_tester)
try
{
   uint32_t day = 0;
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 1;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.2947;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);

   asset pre_balance = get_token_supply();
   for (int i = 0; i < 365 * 3; i++)
   {
      dayCycle("./tests/usage_data/usage_data_50.json", period_start_sec, 5);
      period_start_sec += 86400;
      skipAhead(period_start_sec);
      day++;
      BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerb)));
      // .25 year inflation = 18.8%
      if (i == uint16_t(365 * .25))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 188);
      }
      // .50 year inflation = 15.3%
      if (i == uint16_t(365 * .50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 153);
      }
      // 1 year inflation = 10.4%
      if (i == uint16_t(365 * 1))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 104);
      }
      // 1.75 year inflation = 5.9%
      if (i == uint16_t(365 * 1.75))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 59);
      }
      // 2.50 year inflation = 3.4%
      if (i == uint16_t(365 * 2.50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 34);
      }
      // 3 year inflation = 2.4%
      if (i == uint16_t(365 * 3))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 24);
      }
   }
   asset post_balance = get_token_supply();
   cout << "50% utilization 3 year inflation: " << 100 * float((post_balance.get_amount() - pre_balance.get_amount())) / pre_balance.get_amount() << endl;
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(test_80_percent_inflation, ux_system_tester)
try
{
   uint32_t day = 0;
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 1;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.2947;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);

   asset pre_balance = get_token_supply();
   for (int i = 0; i < 365 * 3; i++)
   {
      dayCycle("./tests/usage_data/usage_data_80.json", period_start_sec, 5);
      period_start_sec += 86400;
      skipAhead(period_start_sec);
      day++;
      BOOST_REQUIRE_EQUAL(success(), nextperiod(N(defproducerb)));
      // .25 year inflation = 25.6%
      if (i == uint16_t(365 * .25))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 256);
      }
      // .50 year inflation = 20.7%
      if (i == uint16_t(365 * .50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 207);
      }
      // 1 year inflation = 13.8%
      if (i == uint16_t(365 * 1))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 138);
      }
      // 1.75 year inflation = 13.8%
      if (i == uint16_t(365 * 1.75))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 77);
      }
      // 2.50 year inflation = 4.4%
      if (i == uint16_t(365 * 2.50))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 44);
      }
      // 3 year inflation = 3.13%
      if (i == uint16_t(365 * 3))
      {
         auto res_history = get_resource_history(day);
         auto inflation = uint16_t(res_history["inflation"].as<float>() * 1000);
         BOOST_REQUIRE_EQUAL(inflation, 313);
      }
   }
   asset post_balance = get_token_supply();
   cout << "80% utilization 3 year inflation: " << 100 * float((post_balance.get_amount() - pre_balance.get_amount())) / pre_balance.get_amount() << endl;
}
FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE(test_random, ux_system_tester)
try
{
   uint32_t day = 0;
   using namespace std;
   transfer(config::system_account_name, N(alice1111111), ram_core_sym::from_string("30.0000"), config::system_account_name);
   produce_blocks(2);
   // jump to UX start date
   skipAhead(getSecondsSinceEpochUTC("2020-10-01 00:00:00"));

   // activate chain and vote for producers
   active_and_vote_producers();
   activatefeat(N(resource));
   produce_blocks(2);

   long period_start_sec = getSecondsSinceEpochUTC("2020-10-01 00:00:00");
   time_point_sec period_start = time_point_sec(period_start_sec);

   uint16_t dataset_batch_size = 5;
   uint16_t oracle_consensus_threshold = 1;
   uint32_t period_seconds = 60 * 60 * 24;
   float initial_value_transfer_rate = 0.1;
   float max_pay_constant = 0.2947;

   // allow transactions to run
   initresource(dataset_batch_size, oracle_consensus_threshold, period_start, period_seconds, initial_value_transfer_rate, max_pay_constant);
   produce_blocks(2);
   dayCycle("./tests/usage_data/usage_data_rand.json", period_start_sec, 5);
}
FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()