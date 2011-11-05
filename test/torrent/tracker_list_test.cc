#include "config.h"

#include "net/address_list.h"

#include "tracker_list_test.h"

namespace std { using namespace tr1; }

CPPUNIT_TEST_SUITE_REGISTRATION(tracker_list_test);

#define TRACKER_SETUP()                                                 \
  torrent::TrackerList tracker_list;                                    \
  int success_counter = 0;                                              \
  int failure_counter = 0;                                              \
  tracker_list.slot_success() = std::bind(&increment_value, &success_counter); \
  tracker_list.slot_failure() = std::bind(&increment_value, &failure_counter);

#define TRACKER_INSERT(group, name)                             \
  TrackerTest* name = new TrackerTest(&tracker_list, "");       \
  tracker_list.insert(group, name);

void increment_value(int* value) { (*value)++; }

bool
TrackerTest::trigger_success() {
  torrent::TrackerList::address_list address_list;

  return trigger_success(&address_list);
}

bool
TrackerTest::trigger_success(torrent::TrackerList::address_list* address_list) {
  if (parent() == NULL || !is_busy() || !is_open())
    return false;

  m_busy = false;
  m_open = !m_close_on_done;
  m_success_time_last = rak::timer::current().seconds();
  parent()->receive_success(this, address_list);

  m_requesting_state = 0;
  return true;
}

bool
TrackerTest::trigger_failure() {
  if (parent() == NULL || !is_busy() || !is_open())
    return false;

  m_busy = false;
  m_open = !m_close_on_done;
  m_failed_time_last = rak::timer::current().seconds();
  parent()->receive_failed(this, "failed");
  m_requesting_state = 0;
  return true;
}

void
tracker_list_test::test_basic() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(tracker_0 == tracker_list[0]);

  CPPUNIT_ASSERT(tracker_list[0]->parent() == &tracker_list);
  CPPUNIT_ASSERT(std::distance(tracker_list.begin_group(0), tracker_list.end_group(0)) == 1);
  CPPUNIT_ASSERT(tracker_list.find_usable(tracker_list.begin()) != tracker_list.end());
}

void
tracker_list_test::test_enable() {
  TRACKER_SETUP();
  int enabled_counter = 0;
  int disabled_counter = 0;

  tracker_list.slot_tracker_enabled() = std::bind(&increment_value, &enabled_counter);
  tracker_list.slot_tracker_disabled() = std::bind(&increment_value, &disabled_counter);

  TRACKER_INSERT(0, tracker_0);
  TRACKER_INSERT(1, tracker_1);
  CPPUNIT_ASSERT(enabled_counter == 2 && disabled_counter == 0);

  tracker_0->enable(); tracker_1->enable();
  CPPUNIT_ASSERT(enabled_counter == 2 && disabled_counter == 0);
  
  tracker_0->disable(); tracker_1->enable();
  CPPUNIT_ASSERT(enabled_counter == 2 && disabled_counter == 1);

  tracker_1->disable(); tracker_0->disable();
  CPPUNIT_ASSERT(enabled_counter == 2 && disabled_counter == 2);

  tracker_0->enable(); tracker_1->enable();
  tracker_0->enable(); tracker_1->enable();
  CPPUNIT_ASSERT(enabled_counter == 4 && disabled_counter == 2);
}

void
tracker_list_test::test_close() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);
  TRACKER_INSERT(0, tracker_1);
  TRACKER_INSERT(0, tracker_2);
  TRACKER_INSERT(0, tracker_3);

  tracker_list.send_state_idx(0, torrent::Tracker::EVENT_NONE);
  tracker_list.send_state_idx(1, torrent::Tracker::EVENT_STARTED);
  tracker_list.send_state_idx(2, torrent::Tracker::EVENT_STOPPED);
  tracker_list.send_state_idx(3, torrent::Tracker::EVENT_COMPLETED);

  CPPUNIT_ASSERT(tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_1->is_busy());
  CPPUNIT_ASSERT(tracker_2->is_busy());
  CPPUNIT_ASSERT(tracker_3->is_busy());

  tracker_list.close_all_excluding((1 << torrent::Tracker::EVENT_STARTED) | (1 << torrent::Tracker::EVENT_STOPPED));

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_1->is_busy());
  CPPUNIT_ASSERT(tracker_2->is_busy());
  CPPUNIT_ASSERT(!tracker_3->is_busy());

  tracker_list.close_all();

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1->is_busy());
  CPPUNIT_ASSERT(!tracker_2->is_busy());
  CPPUNIT_ASSERT(!tracker_3->is_busy());

  tracker_list.send_state_idx(0, torrent::Tracker::EVENT_NONE);
  tracker_list.send_state_idx(1, torrent::Tracker::EVENT_STARTED);
  tracker_list.send_state_idx(2, torrent::Tracker::EVENT_STOPPED);
  tracker_list.send_state_idx(3, torrent::Tracker::EVENT_COMPLETED);

  tracker_list.close_all_excluding((1 << torrent::Tracker::EVENT_NONE) | (1 << torrent::Tracker::EVENT_COMPLETED));

  CPPUNIT_ASSERT(tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1->is_busy());
  CPPUNIT_ASSERT(!tracker_2->is_busy());
  CPPUNIT_ASSERT(tracker_3->is_busy());
}

// Test clear.

void
tracker_list_test::test_tracker_flags() {
  TRACKER_SETUP();
  tracker_list.insert(0, new TrackerTest(&tracker_list, ""));
  tracker_list.insert(0, new TrackerTest(&tracker_list, "", 0));
  tracker_list.insert(0, new TrackerTest(&tracker_list, "", torrent::Tracker::flag_enabled));
  tracker_list.insert(0, new TrackerTest(&tracker_list, "", torrent::Tracker::flag_extra_tracker));
  tracker_list.insert(0, new TrackerTest(&tracker_list, "", torrent::Tracker::flag_enabled | torrent::Tracker::flag_extra_tracker));

  CPPUNIT_ASSERT(tracker_list[0]->flags() == torrent::Tracker::flag_enabled);
  CPPUNIT_ASSERT(tracker_list[1]->flags() == 0);
  CPPUNIT_ASSERT(tracker_list[2]->flags() == torrent::Tracker::flag_enabled);
  CPPUNIT_ASSERT(tracker_list[3]->flags() == torrent::Tracker::flag_extra_tracker);
  CPPUNIT_ASSERT(tracker_list[4]->flags() == torrent::Tracker::flag_enabled | torrent::Tracker::flag_extra_tracker);
}

void
tracker_list_test::test_find_url() {
  TRACKER_SETUP();

  tracker_list.insert(0, new TrackerTest(&tracker_list, "http://1"));
  tracker_list.insert(0, new TrackerTest(&tracker_list, "http://2"));
  tracker_list.insert(1, new TrackerTest(&tracker_list, "http://3"));

  CPPUNIT_ASSERT(tracker_list.find_url("http://") == tracker_list.end());

  CPPUNIT_ASSERT(tracker_list.find_url("http://1") != tracker_list.end());
  CPPUNIT_ASSERT(*tracker_list.find_url("http://1") == tracker_list[0]);

  CPPUNIT_ASSERT(tracker_list.find_url("http://2") != tracker_list.end());
  CPPUNIT_ASSERT(*tracker_list.find_url("http://2") == tracker_list[1]);

  CPPUNIT_ASSERT(tracker_list.find_url("http://3") != tracker_list.end());
  CPPUNIT_ASSERT(*tracker_list.find_url("http://3") == tracker_list[2]);
}

void
tracker_list_test::test_single_success() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == -1);
  CPPUNIT_ASSERT(tracker_0->latest_event() == torrent::Tracker::EVENT_NONE);

  tracker_list.send_state_idx(0, torrent::Tracker::EVENT_STARTED);

  CPPUNIT_ASSERT(tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == torrent::Tracker::EVENT_STARTED);
  CPPUNIT_ASSERT(tracker_0->latest_event() == torrent::Tracker::EVENT_STARTED);

  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == 0);
  CPPUNIT_ASSERT(tracker_0->latest_event() == torrent::Tracker::EVENT_STARTED);
  
  CPPUNIT_ASSERT(success_counter == 1 && failure_counter == 0);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 1);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 0);
}

void
tracker_list_test::test_single_failure() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  tracker_list.send_state_idx(0, 1);
  CPPUNIT_ASSERT(tracker_0->trigger_failure());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->requesting_state() == 0);

  CPPUNIT_ASSERT(success_counter == 0 && failure_counter == 1);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 0);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 1);

  tracker_list.send_state_idx(0, 1);
  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(success_counter == 1 && failure_counter == 1);
  CPPUNIT_ASSERT(tracker_0->success_counter() == 1);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 0);
}

void
tracker_list_test::test_single_closing() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0);

  CPPUNIT_ASSERT(!tracker_0->is_open());

  tracker_0->set_close_on_done(false);
  tracker_list.send_state_idx(0, 1);

  CPPUNIT_ASSERT(tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->trigger_success());

  CPPUNIT_ASSERT(!tracker_0->is_busy());
  CPPUNIT_ASSERT(tracker_0->is_open());

  tracker_list.close_all();
  tracker_list.clear_stats();

  CPPUNIT_ASSERT(!tracker_0->is_open());
  CPPUNIT_ASSERT(tracker_0->success_counter() == 0);
  CPPUNIT_ASSERT(tracker_0->failed_counter() == 0);
}

void
tracker_list_test::test_multiple_success() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0_0);
  TRACKER_INSERT(0, tracker_0_1);
  TRACKER_INSERT(1, tracker_1_0);
  TRACKER_INSERT(1, tracker_1_1);

  CPPUNIT_ASSERT(!tracker_0_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1_1->is_busy());

  tracker_list.send_state_idx(0, 1);

  CPPUNIT_ASSERT(tracker_0_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1_1->is_busy());

  CPPUNIT_ASSERT(tracker_0_0->trigger_success());

  CPPUNIT_ASSERT(!tracker_0_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1_1->is_busy());

  tracker_list.send_state_idx(1, 1);
  tracker_list.send_state_idx(3, 1);

  CPPUNIT_ASSERT(!tracker_0_0->is_busy());
  CPPUNIT_ASSERT(tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(tracker_1_1->is_busy());

  CPPUNIT_ASSERT(tracker_1_1->trigger_success());

  CPPUNIT_ASSERT(!tracker_0_0->is_busy());
  CPPUNIT_ASSERT(tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1_1->is_busy());

  CPPUNIT_ASSERT(tracker_0_1->trigger_success());

  CPPUNIT_ASSERT(!tracker_0_0->is_busy());
  CPPUNIT_ASSERT(!tracker_0_1->is_busy());
  CPPUNIT_ASSERT(!tracker_1_0->is_busy());
  CPPUNIT_ASSERT(!tracker_1_1->is_busy());

  CPPUNIT_ASSERT(success_counter == 3 && failure_counter == 0);
}


// test last_connect timer.


// test has_active, and then clean up TrackerManager.

void
tracker_list_test::test_has_active() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0_0);
  TRACKER_INSERT(0, tracker_0_1);
  TRACKER_INSERT(1, tracker_1_0);

  CPPUNIT_ASSERT(!tracker_list.has_active());

  tracker_list.send_state_idx(0, 1); CPPUNIT_ASSERT(tracker_list.has_active());
  tracker_0_0->trigger_success(); CPPUNIT_ASSERT(!tracker_list.has_active());
  
  tracker_list.send_state_idx(2, 1); CPPUNIT_ASSERT(tracker_list.has_active());
  tracker_1_0->trigger_success(); CPPUNIT_ASSERT(!tracker_list.has_active());

  // Test multiple active trackers.
  tracker_list.send_state_idx(0, 1); CPPUNIT_ASSERT(tracker_list.has_active());

  tracker_list.send_state_idx(1, 1);
  tracker_0_0->trigger_success(); CPPUNIT_ASSERT(tracker_list.has_active());
  tracker_0_1->trigger_success(); CPPUNIT_ASSERT(!tracker_list.has_active());
}

void
tracker_list_test::test_count_active() {
  TRACKER_SETUP();
  TRACKER_INSERT(0, tracker_0_0);
  TRACKER_INSERT(0, tracker_0_1);
  TRACKER_INSERT(1, tracker_1_0);
  TRACKER_INSERT(2, tracker_2_0);

  CPPUNIT_ASSERT(tracker_list.count_active() == 0);

  tracker_list.send_state_idx(0, 1);
  CPPUNIT_ASSERT(tracker_list.count_active() == 1);

  tracker_list.send_state_idx(3, 1);
  CPPUNIT_ASSERT(tracker_list.count_active() == 2);

  tracker_list.send_state_idx(1, 1);
  tracker_list.send_state_idx(2, 1);
  CPPUNIT_ASSERT(tracker_list.count_active() == 4);

  tracker_0_0->trigger_success();
  CPPUNIT_ASSERT(tracker_list.count_active() == 3);
  
  tracker_0_1->trigger_success();
  tracker_2_0->trigger_success();
  CPPUNIT_ASSERT(tracker_list.count_active() == 1);
  
  tracker_1_0->trigger_success();
  CPPUNIT_ASSERT(tracker_list.count_active() == 0);
}

// Add separate functions for sending state to multiple trackers...