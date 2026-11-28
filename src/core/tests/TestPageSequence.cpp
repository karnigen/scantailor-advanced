// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include <ImageId.h>
#include <ImageInfo.h>
#include <ImageTransformation.h>
#include <PageId.h>
#include <PageInfo.h>
#include <PageSequence.h>
#include <ProcessingTaskQueue.h>
#include <ProjectPages.h>
#include <filters/page_split/CacheDrivenTask.h>
#include <filters/page_split/Settings.h>

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <set>
#include <vector>

namespace Tests {

static PageInfo makePage(const char* path, const int imageIdx) {
  PageInfo info;
  info.setId(PageId(ImageId(path, imageIdx), PageId::SINGLE_PAGE));
  return info;
}

class CompletedBatchTask : public BackgroundTask {
 public:
  CompletedBatchTask() : BackgroundTask(BATCH) {}
  FilterResultPtr operator()() override { return nullptr; }
  FilterResultPtr exceptionResult(const QString&) override { return nullptr; }
};

TEST_CASE("PageSequenceTestSuite / select_this_page_and_following_every_other_from_first") {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const PageId base = pages[0].id();
  const std::set<PageId> result = seq.selectThisPageAndFollowingEveryOther(base);

  CHECK((result.size()) == (38u));
  for (int i = 0; i < 75; i += 2) {
    CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 1);
  }
  for (int i = 1; i < 75; i += 2) {
    CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 0);
  }
}

TEST_CASE("PageSequenceTestSuite / select_this_page_and_following_every_other_from_second") {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const PageId base = pages[1].id();
  const std::set<PageId> result = seq.selectThisPageAndFollowingEveryOther(base);

  CHECK((result.size()) == (37u));
  for (int i = 1; i < 75; i += 2) {
    CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 1);
  }
  for (int i = 0; i < 75; i += 2) {
    CHECK(result.count(pages[static_cast<size_t>(i)].id()) == 0);
  }
}

TEST_CASE("PageSequenceTestSuite / select_this_page_and_following_every_other_partitions_75_pages") {
  PageSequence seq;
  std::vector<PageInfo> pages;
  for (int i = 0; i < 75; ++i) {
    pages.push_back(makePage("/scan", i));
    seq.append(pages.back());
  }

  const std::set<PageId> oddFromFirst = seq.selectThisPageAndFollowingEveryOther(pages[0].id());
  const std::set<PageId> evenFromSecond = seq.selectThisPageAndFollowingEveryOther(pages[1].id());

  for (int i = 0; i < 75; ++i) {
    const bool inOdd = oddFromFirst.count(pages[static_cast<size_t>(i)].id()) != 0;
    const bool inEven = evenFromSecond.count(pages[static_cast<size_t>(i)].id()) != 0;
    CHECK(inOdd != inEven);
  }
}

TEST_CASE("ProjectPages / cached split cannot overwrite a newer worker layout") {
  const ImageId imageId("/page.tif", 0);
  const ImageMetadata oldMetadata(QSize(100, 200), Dpi(300, 300));
  const ImageMetadata newMetadata(QSize(200, 100), Dpi(300, 300));
  ProjectPages pages({ImageInfo(imageId, oldMetadata, 1, false, false)}, Qt::LeftToRight);
  const PageInfo oldPage = pages.toPageSequence(PAGE_VIEW).pageAt(0);
  const auto cachedRevision = pages.layoutRevision(oldPage);
  REQUIRE(cachedRevision);
  ProcessingTaskQueue queue;
  const auto task = std::make_shared<CompletedBatchTask>();
  queue.addProcessingTask(oldPage, task);
  REQUIRE(queue.takeForProcessing() == task);

  pages.updateImageMetadata(imageId, newMetadata);
  pages.setLayoutTypeFor(imageId, ProjectPages::TWO_PAGE_LAYOUT);

  CHECK_FALSE(pages.setLayoutTypeForIfUnchanged(imageId, ProjectPages::ONE_PAGE_LAYOUT, *cachedRevision));
  CHECK_FALSE(pages.layoutRevision(oldPage));
  queue.processingFinished(task);
  REQUIRE(queue.allProcessed());
  const PageSequence completed = pages.toPageSequence(PAGE_VIEW);
  REQUIRE(completed.numPages() == 2);
  CHECK(completed.pageAt(0).id().subPage() == PageId::LEFT_PAGE);
  CHECK(completed.pageAt(1).id().subPage() == PageId::RIGHT_PAGE);
}

TEST_CASE("ProjectPages / cached split rejects a newer layout even when page count did not change") {
  const ImageId imageId("/page.tif", 0);
  const ImageMetadata metadata(QSize(100, 200), Dpi(300, 300));
  ProjectPages pages({ImageInfo(imageId, metadata, 1, false, false)}, Qt::LeftToRight);
  const PageInfo page = pages.toPageSequence(PAGE_VIEW).pageAt(0);
  const auto cachedRevision = pages.layoutRevision(page);
  REQUIRE(cachedRevision);

  pages.setLayoutTypeFor(imageId, ProjectPages::ONE_PAGE_LAYOUT);
  CHECK_FALSE(pages.setLayoutTypeForIfUnchanged(imageId, ProjectPages::TWO_PAGE_LAYOUT, *cachedRevision));
  CHECK(pages.toPageSequence(PAGE_VIEW).numPages() == 1);

  const auto currentRevision = pages.layoutRevision(page);
  REQUIRE(currentRevision);
  CHECK(pages.setLayoutTypeForIfUnchanged(imageId, ProjectPages::TWO_PAGE_LAYOUT, *currentRevision));
  CHECK(pages.toPageSequence(PAGE_VIEW).numPages() == 2);
}

TEST_CASE("ProjectPages / per-image reading direction invalidates only its cached split") {
  const ImageId firstId("/first.tif", 0);
  const ImageId secondId("/second.tif", 0);
  const ImageMetadata metadata(QSize(200, 100), Dpi(300, 300));
  ProjectPages pages({ImageInfo(firstId, metadata, 2, false, false), ImageInfo(secondId, metadata, 2, false, false)},
                     Qt::LeftToRight);
  const PageSequence initial = pages.toPageSequence(PAGE_VIEW);
  const auto firstRevision = pages.layoutRevision(initial.pageAt(0));
  const auto secondRevision = pages.layoutRevision(initial.pageAt(2));
  REQUIRE(firstRevision);
  REQUIRE(secondRevision);

  pages.setLayoutDirectionFor(firstId, Qt::RightToLeft);

  CHECK_FALSE(pages.setLayoutTypeForIfUnchanged(firstId, ProjectPages::TWO_PAGE_LAYOUT, *firstRevision));
  CHECK(pages.setLayoutTypeForIfUnchanged(secondId, ProjectPages::TWO_PAGE_LAYOUT, *secondRevision));
  const PageSequence updated = pages.toPageSequence(PAGE_VIEW);
  CHECK(updated.pageAt(0).id().subPage() == PageId::RIGHT_PAGE);
  CHECK(updated.pageAt(1).id().subPage() == PageId::LEFT_PAGE);
  CHECK(updated.pageAt(2).id().subPage() == PageId::LEFT_PAGE);
  CHECK(updated.pageAt(3).id().subPage() == PageId::RIGHT_PAGE);
}

TEST_CASE("ProjectPages / project reading direction invalidates only images without overrides") {
  const ImageId defaultDirectionId("/default.tif", 0);
  const ImageId overrideDirectionId("/override.tif", 0);
  const ImageMetadata metadata(QSize(200, 100), Dpi(300, 300));
  ProjectPages pages({ImageInfo(defaultDirectionId, metadata, 2, false, false),
                      ImageInfo(overrideDirectionId, metadata, 2, false, false)},
                     Qt::LeftToRight);
  pages.setLayoutDirectionFor(overrideDirectionId, Qt::RightToLeft);
  const PageSequence initial = pages.toPageSequence(PAGE_VIEW);
  const auto defaultRevision = pages.layoutRevision(initial.pageAt(0));
  const auto overrideRevision = pages.layoutRevision(initial.pageAt(2));
  REQUIRE(defaultRevision);
  REQUIRE(overrideRevision);

  pages.setLayoutDirection(Qt::RightToLeft);

  CHECK_FALSE(pages.setLayoutTypeForIfUnchanged(defaultDirectionId, ProjectPages::TWO_PAGE_LAYOUT, *defaultRevision));
  CHECK(pages.setLayoutTypeForIfUnchanged(overrideDirectionId, ProjectPages::TWO_PAGE_LAYOUT, *overrideRevision));
  const PageSequence updated = pages.toPageSequence(PAGE_VIEW);
  CHECK(updated.pageAt(0).id().subPage() == PageId::RIGHT_PAGE);
  CHECK(updated.pageAt(1).id().subPage() == PageId::LEFT_PAGE);
  CHECK(updated.pageAt(2).id().subPage() == PageId::RIGHT_PAGE);
  CHECK(updated.pageAt(3).id().subPage() == PageId::LEFT_PAGE);
}

TEST_CASE("ProjectPages / another image does not invalidate a cached split") {
  const ImageId firstId("/first.tif", 0);
  const ImageId secondId("/second.tif", 0);
  const ImageMetadata metadata(QSize(100, 200), Dpi(300, 300));
  ProjectPages pages({ImageInfo(firstId, metadata, 1, false, false), ImageInfo(secondId, metadata, 1, false, false)},
                     Qt::LeftToRight);
  const PageInfo firstPage = pages.toPageSequence(PAGE_VIEW).pageAt(0);
  const auto revision = pages.layoutRevision(firstPage);
  REQUIRE(revision);
  pages.setLayoutTypeFor(secondId, ProjectPages::TWO_PAGE_LAYOUT);

  CHECK(pages.setLayoutTypeForIfUnchanged(firstId, ProjectPages::TWO_PAGE_LAYOUT, *revision));
  CHECK(pages.toPageSequence(PAGE_VIEW).numPages() == 4);
}

TEST_CASE("PageSplitCacheDrivenTask / stale page metadata cannot restore an old split") {
  const ImageId imageId("/page.tif", 0);
  const ImageMetadata oldMetadata(QSize(100, 200), Dpi(300, 300));
  const ImageMetadata newMetadata(QSize(200, 100), Dpi(300, 300));
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(imageId, oldMetadata, 1, false, false)}, Qt::LeftToRight);
  const PageInfo oldPage = pages->toPageSequence(PAGE_VIEW).pageAt(0);
  const auto settings = std::make_shared<page_split::Settings>();
  const QRectF oldRect(0, 0, 100, 200);
  page_split::Settings::UpdateAction update;
  update.setLayoutType(page_split::SINGLE_PAGE_UNCUT);
  update.setParams(page_split::Params(
      page_split::PageLayout(oldRect),
      page_split::Dependencies(oldMetadata.size(), OrthogonalRotation(), page_split::SINGLE_PAGE_UNCUT), MODE_MANUAL));
  settings->updatePage(imageId, update);
  pages->updateImageMetadata(imageId, newMetadata);
  pages->setLayoutTypeFor(imageId, ProjectPages::TWO_PAGE_LAYOUT);

  page_split::CacheDrivenTask cache(settings, pages, nullptr);
  cache.process(oldPage, nullptr, ImageTransformation(oldRect, Dpi(300, 300)));

  CHECK(pages->toPageSequence(PAGE_VIEW).numPages() == 2);
}

TEST_CASE("PageSplitCacheDrivenTask / current cached split updates the page count") {
  const ImageId imageId("/page.tif", 0);
  const ImageMetadata metadata(QSize(200, 100), Dpi(300, 300));
  const auto pages = std::make_shared<ProjectPages>(
      std::vector<ImageInfo>{ImageInfo(imageId, metadata, 1, false, false)}, Qt::LeftToRight);
  const PageInfo page = pages->toPageSequence(PAGE_VIEW).pageAt(0);
  const auto settings = std::make_shared<page_split::Settings>();
  const QRectF rect(0, 0, 200, 100);
  const page_split::PageLayout layout(rect, QLineF(100, 0, 100, 100));
  page_split::Settings::UpdateAction update;
  update.setLayoutType(page_split::TWO_PAGES);
  update.setParams(page_split::Params(
      layout, page_split::Dependencies(metadata.size(), OrthogonalRotation(), page_split::TWO_PAGES), MODE_MANUAL));
  settings->updatePage(imageId, update);

  page_split::CacheDrivenTask cache(settings, pages, nullptr);
  cache.process(page, nullptr, ImageTransformation(rect, Dpi(300, 300)));

  CHECK(pages->toPageSequence(PAGE_VIEW).numPages() == 2);
}

}  // namespace Tests
