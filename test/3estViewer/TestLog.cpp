//
// author: Kazys Stepanas
//
#include "3estViewer/TestViewerConfig.h"

#include "TestViewer.h"

#include <3esview/ViewerLog.h>

#include <array>
#include <string>

namespace tes::view
{
TEST(Log, View)
{
  // Make a log with a small number of max lines
  const size_t log_size = 10;
  // Log 10 times as many items as the log size.
  const size_t log_range = 10 * log_size;
  ViewerLog log(log_size);

  const auto validate_log = [&log, log_size](size_t cursor) {
    size_t expected_value = (cursor >= log_size) ? cursor - log_size + 1 : 0;
    bool ok = true;
    const auto view = log.view();
    const auto end = view.end();
    for (auto iter = view.begin(); iter != end; ++iter, ++expected_value)
    {
      const auto &log_entry = *iter;
      const auto expected_str = std::to_string(expected_value);
      EXPECT_EQ(log_entry.message, expected_str);
    }
    return ok;
  };

  // Fill the buffer up, validating the content on each added item.
  const auto level = log::Level::Trace;
  for (size_t i = 0; i < log_range; ++i)
  {
    log.log(level, std::to_string(i));
    validate_log(i);
  }
}

TEST(Log, ViewFilter)
{
  // Make a log with a small number of max lines
  const size_t log_size = 10;
  // Log 10 times as many items as the log size.
  const size_t log_range = 10 * log_size;
  ViewerLog log(log_size);

  // Test filtered log views.
  const auto next_level = [](log::Level level) {
    return static_cast<log::Level>((static_cast<int>(level) + 1) %
                                   (static_cast<int>(log::Level::Trace) + 1));
  };


  const auto validate = [&log, &next_level]() {
    using Histogram = std::array<unsigned, static_cast<int>(log::Level::Trace) + 1>;
    Histogram histogram = {};

    // Use a full view to build a histogram of messages present.
    // Scope to release the view so we can get another.
    {
      auto view = log.view();
      for (auto iter = view.begin(); iter != view.end(); ++iter)
      {
        const auto level_index = static_cast<unsigned>(iter->level);
        ++histogram[level_index];
      }
    }

    // Now get filtered logs and check their item counts.
    bool begin = true;
    for (auto level = log::Level::Fatal; begin || level != log::Level::Fatal;
         level = next_level(level), begin = false)
    {
      auto view = log.view(level);
      const auto view_size = view.size();
      size_t expected_size = 0;

      for (int l = 0; l <= static_cast<int>(level) && l < histogram.size(); ++l)
      {
        expected_size += histogram[l];
      }

      EXPECT_EQ(view_size, expected_size);

      // No visit each item.
      size_t visit_size = 0;
      const auto end = view.end();
      for (auto iter = view.begin(); iter != end; ++iter)
      {
        const auto &log_entry = *iter;
        EXPECT_LE(static_cast<int>(log_entry.level), static_cast<int>(level));
        ++visit_size;
      }

      EXPECT_EQ(visit_size, expected_size);
    }
  };

  auto level = log::Level::Fatal;
  // Add log items.
  for (size_t i = 0; i < log_range; ++i)
  {
    log.log(level, std::to_string(i));
    level = next_level(level);
    validate();
  }
}
}  // namespace tes::view
