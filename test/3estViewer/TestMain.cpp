#include "3estViewer/TestViewerConfig.h"

#include "TestViewer.h"

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
  // Workaround: See createViewer() documentation.
  auto viewer = tes::view::TestViewer::createViewer();
  return RUN_ALL_TESTS();
}
