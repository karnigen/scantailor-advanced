// SPDX-FileCopyrightText: 2026 ScanTailor Advanced contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <Dpi.h>
#include <PageId.h>
#include <PathUtils.h>
#include <filters/deskew/ImageView.h>
#include <filters/deskew/OptionsWidget.h>
#include <filters/deskew/Settings.h>

#include <QAction>
#include <QDoubleSpinBox>
#include <QLineF>
#include <QPainter>
#include <QScrollBar>
#include <QSignalSpy>
#include <QWheelEvent>
#include <QtTest/QtTest>

#include <cmath>
#include <memory>

#include "PageSelectionAccessor.h"

class TestDeskewQt : public QObject {
  Q_OBJECT

 private slots:
  void testAngleWheelAppliesFineRotation();
  void testRotationControlsPreserveVisualScale();
  void testGridRemainsStableDuringRotation();
  void testCtrlWheelAppliesFineRotation();
};

class TestDeskewImageView : public deskew::ImageView {
 public:
  using deskew::ImageView::ImageView;

  QImage renderOverlay() {
    QImage overlay(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
    overlay.fill(Qt::transparent);
    QPainter painter(&overlay);
    onPaint(painter, interactionState());
    return overlay;
  }
};

void TestDeskewQt::testGridRemainsStableDuringRotation() {
  QImage image(600, 900, QImage::Format_RGB32);
  image.fill(Qt::white);
  const ImageTransformation xform(QRectF(image.rect()), Dpi(300, 300));
  TestDeskewImageView view(image, image, xform);
  view.resize(800, 600);
  view.show();
  view.activateWindow();
  view.viewport()->setFocus();
  QTRY_VERIFY(view.viewport()->width() == 800);
  QTRY_VERIFY(view.isActiveWindow());

  deskew::OptionsWidget widget(std::make_shared<deskew::Settings>(), PageSelectionAccessor(nullptr));
  const PageId pageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/deskew-page.tif"))), PageId::SINGLE_PAGE);
  widget.preUpdateUI(pageId);
  widget.postUpdateUI(deskew::OptionsWidget::UiData());
  QObject::connect(&view, &deskew::ImageView::manualDeskewAngleSet, &widget,
                   &deskew::OptionsWidget::manualDeskewAngleSetExternally);
  QObject::connect(&widget, &deskew::OptionsWidget::manualDeskewAngleSet, &view,
                   &deskew::ImageView::manualDeskewAngleSetExternally);

  const auto gridCenter = [&view] {
    const QImage overlay = view.renderOverlay();
    int lineX = -1;
    int lineY = -1;
    for (int x = 390; x < 410; ++x) {
      if (overlay.pixel(x, 230) == qRgba(0, 0, 209, 255)) {
        lineX = x;
        break;
      }
    }
    for (int y = 290; y < 310; ++y) {
      if (overlay.pixel(330, y) == qRgba(0, 0, 209, 255)) {
        lineY = y;
        break;
      }
    }
    return QPoint(lineX, lineY);
  };
  const QPoint initialGridCenter = gridCenter();
  QVERIFY(initialGridCenter.x() >= 0);
  QVERIFY(initialGridCenter.y() >= 0);
  QSignalSpy rotationSpy(&view, &deskew::ImageView::manualDeskewAngleSet);
  for (int i = 0; i < 20; ++i) {
    QTest::keyClick(view.viewport(), Qt::Key_Period);
    QCOMPARE(rotationSpy.count(), i + 1);
    QCOMPARE(gridCenter(), initialGridCenter);
  }
  for (int i = 0; i < 20; ++i) {
    QTest::keyClick(view.viewport(), Qt::Key_Comma);
    QCOMPARE(rotationSpy.count(), i + 21);
    QCOMPARE(gridCenter(), initialGridCenter);
  }

  view.setWidgetFocalPoint(view.getWidgetFocalPoint() + QPointF(300, 0));
  QVERIFY(view.getOccupiedWidgetRect().left() > initialGridCenter.x());
  const QImage pannedOverlay = view.renderOverlay();
  QCOMPARE(qAlpha(pannedOverlay.pixel(480, 230)), 0);
  QVERIFY(qAlpha(pannedOverlay.pixel(540, 230)) > 0);
}

void TestDeskewQt::testAngleWheelAppliesFineRotation() {
  const PageId pageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/deskew-page.tif"))), PageId::SINGLE_PAGE);
  auto settings = std::make_shared<deskew::Settings>();
  deskew::OptionsWidget widget(settings, PageSelectionAccessor(nullptr));
  widget.preUpdateUI(pageId);
  widget.postUpdateUI(deskew::OptionsWidget::UiData());
  widget.show();

  auto* angleSpinBox = widget.findChild<QDoubleSpinBox*>(QStringLiteral("angleSpinBox"));
  QVERIFY(angleSpinBox);
  QVERIFY(!widget.findChild<QObject*>(QStringLiteral("rotateLeftBtn")));
  QVERIFY(!widget.findChild<QObject*>(QStringLiteral("rotateRightBtn")));

  QSignalSpy rotationSpy(&widget, &deskew::OptionsWidget::manualDeskewAngleSet);
  QSignalSpy reloadSpy(&widget, &FilterOptionsWidget::reloadRequested);
  const QPointF position = angleSpinBox->rect().center();
  QWheelEvent wheelUp(position, angleSpinBox->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, 120), Qt::NoButton,
                      Qt::NoModifier, Qt::NoScrollPhase, false);
  QApplication::sendEvent(angleSpinBox, &wheelUp);
  QVERIFY(std::abs(angleSpinBox->value() - 0.1) < 1e-9);
  QCOMPARE(rotationSpy.count(), 1);
  QVERIFY(std::abs(rotationSpy.takeFirst().at(0).toDouble() + 0.1) < 1e-9);

  QWheelEvent wheelDown(position, angleSpinBox->mapToGlobal(position.toPoint()), QPoint(), QPoint(0, -120),
                        Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
  QApplication::sendEvent(angleSpinBox, &wheelDown);
  QVERIFY(std::abs(angleSpinBox->value()) < 1e-9);
  QCOMPARE(rotationSpy.count(), 1);
  QVERIFY(std::abs(rotationSpy.takeFirst().at(0).toDouble()) < 1e-9);
  QCOMPARE(reloadSpy.count(), 0);
  QVERIFY(settings->getPageParams(pageId));
  QTRY_COMPARE(reloadSpy.count(), 1);
  QVERIFY(settings->getPageParams(pageId));
  QTest::qWait(300);
  QCOMPARE(reloadSpy.count(), 1);
}

void TestDeskewQt::testRotationControlsPreserveVisualScale() {
  QImage image(100, 100, QImage::Format_RGB32);
  image.fill(Qt::white);
  const ImageTransformation xform(QRectF(image.rect()), Dpi(300, 300));
  deskew::ImageView view(image, image, xform);
  view.resize(300, 300);
  view.show();
  QTRY_VERIFY(view.viewport()->width() > 0);
  view.setZoomLevel(4.0);

  deskew::OptionsWidget widget(std::make_shared<deskew::Settings>(), PageSelectionAccessor(nullptr));
  const PageId pageId(ImageId(core::toFileSystemPath(QStringLiteral("/tmp/deskew-page.tif"))), PageId::SINGLE_PAGE);
  widget.preUpdateUI(pageId);
  widget.postUpdateUI(deskew::OptionsWidget::UiData());
  widget.show();
  QObject::connect(&widget, &deskew::OptionsWidget::manualDeskewAngleSet, &view,
                   &deskew::ImageView::manualDeskewAngleSetExternally);

  const auto horizontalImageScale = [&view] {
    const QTransform transform = view.imageToWidget();
    return QLineF(transform.map(QPointF(20.0, 50.0)), transform.map(QPointF(80.0, 50.0))).length();
  };

  auto* angleSpinBox = widget.findChild<QDoubleSpinBox*>(QStringLiteral("angleSpinBox"));
  QVERIFY(angleSpinBox);
  double scaleBefore = horizontalImageScale();
  angleSpinBox->setValue(0.1);
  QVERIFY(std::abs(horizontalImageScale() - scaleBefore) < 1e-5);

  auto* rotateRightAction = view.findChild<QAction*>(QStringLiteral("deskewRotateRight"));
  QVERIFY(rotateRightAction);
  scaleBefore = horizontalImageScale();
  rotateRightAction->trigger();
  QVERIFY(std::abs(horizontalImageScale() - scaleBefore) < 1e-5);
}

void TestDeskewQt::testCtrlWheelAppliesFineRotation() {
  QImage image(100, 100, QImage::Format_RGB32);
  image.fill(Qt::white);
  const ImageTransformation xform(QRectF(image.rect()), Dpi(300, 300));
  deskew::ImageView view(image, image, xform);
  view.resize(300, 300);
  view.show();
  QTRY_VERIFY(view.viewport()->width() > 0);

  view.setZoomLevel(4.0);
  QCOMPARE(view.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
  QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
  QVERIFY(!view.horizontalScrollBar()->isVisible());
  QVERIFY(!view.verticalScrollBar()->isVisible());

  QSignalSpy rotationSpy(&view, &deskew::ImageView::manualDeskewAngleSet);
  const QPointF position(100, 100);
  QWheelEvent wheelEvent(position, view.mapToGlobal(position.toPoint()), QPoint(), QPoint(0, 120), Qt::NoButton,
                         Qt::ControlModifier | Qt::KeypadModifier, Qt::NoScrollPhase, false);
  QApplication::sendEvent(view.viewport(), &wheelEvent);

  QCOMPARE(rotationSpy.count(), 1);
  QVERIFY(std::abs(rotationSpy.takeFirst().at(0).toDouble() + 0.1) < 1e-9);
  QCOMPARE(view.horizontalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
  QCOMPARE(view.verticalScrollBarPolicy(), Qt::ScrollBarAlwaysOff);
  QVERIFY(!view.horizontalScrollBar()->isVisible());
  QVERIFY(!view.verticalScrollBar()->isVisible());
}

QTEST_MAIN(TestDeskewQt)

#include "tst_deskew_qt.moc"
