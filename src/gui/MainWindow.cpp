#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <iterator>   // std::size

using namespace egg;

namespace {

// The action list offered per button. Covers what the hardware exposes without
// pretending to be a full HID key picker.
struct Action {
    const char* label;
    ActionType  type;
    uint8_t     code;
};

const Action kActions[] = {
    {"Left click",     ActionType::MouseButton, 0x01},
    {"Right click",    ActionType::MouseButton, 0x02},
    {"Middle click",   ActionType::MouseButton, 0x04},
    {"Back",           ActionType::MouseButton, 0x08},
    {"Forward",        ActionType::MouseButton, 0x10},
    {"Wheel up",       ActionType::Wheel,       0x01},
    {"Wheel down",     ActionType::Wheel,       0xFF},
    {"CPI cycle",      ActionType::Special,     0xF1},
    {"Volume up",      ActionType::Consumer,    0xE9},
    {"Volume down",    ActionType::Consumer,    0xEA},
    {"Mute",           ActionType::Consumer,    0xE2},
    {"Play / Pause",   ActionType::Consumer,    0xCD},
    {"Next track",     ActionType::Consumer,    0xB5},
    {"Previous track", ActionType::Consumer,    0xB6},
    // The vendor MEDIA submenu also offers BROWSER and EXPLORER; these are the
    // standard HID consumer usages for them, and UnofficialEGGMouseConfig uses
    // the same two values. Consumer bindings with arbitrary usage codes are
    // already proven to work on this model.
    {"Browser",        ActionType::Consumer,    0x96},
    {"File manager",   ActionType::Consumer,    0x94},
    // Picking this opens a dialog for the CPI value; it is a command, never a
    // state, so an existing binding always renders through describe() instead.
    {"Fixed CPI…",     ActionType::CpiPreset,   0x00},
    {"Disabled",       ActionType::Disabled,    0x00},
};

constexpr int kActionCount = static_cast<int>(std::size(kActions));

int indexOfAction(const ButtonEntry& e)
{
    for (int i = 0; i < kActionCount; ++i) {
        if (kActions[i].type == ActionType::CpiPreset) {
            continue;   // the dialog entry, not something a binding can equal
        }
        if (static_cast<uint8_t>(kActions[i].type) == e.type && kActions[i].code == e.code) {
            return i;
        }
    }
    return -1;   // something we do not offer, e.g. a keyboard or fixed-CPI binding
}

// The swatch doubles as the active-stage marker. It has to, for the v1: there
// the stage buttons are disabled, and a disabled checked button is too faint to
// read in several styles.
void applySwatchStyle(QLabel* swatch, size_t stage, bool active)
{
    const StageColour& c = kStageColours[stage];
    swatch->setStyleSheet(
        QStringLiteral("background-color: rgb(%1, %2, %3); border: %4px solid %5;")
            .arg(int(c.r))
            .arg(int(c.g))
            .arg(int(c.b))
            .arg(active ? 3 : 1)
            .arg(active ? QStringLiteral("palette(highlight)")
                        : QStringLiteral("palette(mid)")));
}

// Named separately from kStageColours so the word is translatable; the RGB
// values stay in one place, in protocol.h.
QString stageColourName(size_t stage)
{
    switch (stage) {
    case 0:  return QObject::tr("blue");
    case 1:  return QObject::tr("green");
    case 2:  return QObject::tr("yellow");
    default: return QObject::tr("red");
    }
}

}  // namespace

MainWindow::MainWindow()
{
    // Empty until the mouse names itself, so the window shows the application
    // display name alone; repopulateForModel() fills in the model. Hardcoding a
    // model here would mislabel the other one.
    setWindowTitle(QString());

    auto* central = new QWidget;
    auto* layout  = new QVBoxLayout(central);
    layout->addWidget(buildInfoPanel());

    auto* tabs = new QTabWidget;
    tabs->addTab(buildBasicTab(),    tr("Basic Settings"));
    tabs->addTab(buildAdvancedTab(), tr("Advanced Settings"));
    tabs->addTab(buildButtonsTab(),  tr("Button Mapping"));
    layout->addWidget(tabs, 1);

    setCentralWidget(central);
    statusLabel_ = new QLabel;
    statusBar()->addWidget(statusLabel_);

    // Nothing has been read yet, so nothing may be written yet.
    setConfigLoaded(false);

    // The dongle pushes a notification when the radio link changes state, so
    // nothing here is polled over USB. This timer only drains the local hidraw
    // queue, which costs nothing; the device is touched solely in response to
    // an actual event, or when the user asks. See PROTOCOL.md section 4a.
    eventTimer_ = new QTimer(this);
    eventTimer_->setInterval(1000);
    connect(eventTimer_, &QTimer::timeout, this, &MainWindow::pollEvents);

    if (!device_.open()) {
        report(QString::fromStdString(device_.lastError()), true);
        QMessageBox::critical(this, tr("No device"),
            tr("%1\n\nIf the dongle is plugged in, this is usually a permissions "
               "problem — install udev/70-endgamegear.rules and replug.")
                .arg(QString::fromStdString(device_.lastError())));
        central->setEnabled(false);
        return;
    }

    reload();
    eventTimer_->start();
    resize(760, 720);
}

// ------------------------------------------------------------------ build ---

QWidget* MainWindow::buildInfoPanel()
{
    auto* box  = new QGroupBox(tr("Info"));
    auto* grid = new QGridLayout(box);

    connectionLabel_ = new QLabel(tr("—"));
    batteryLabel_    = new QLabel(tr("—"));
    signalLabel_     = new QLabel(tr("—"));
    mouseFwLabel_    = new QLabel(tr("—"));
    dongleFwLabel_   = new QLabel(tr("—"));
    signalLabel_->setToolTip(
        tr("Reported alongside battery level. The unit is not documented; it "
           "moves around as the mouse is used."));

    grid->addWidget(new QLabel(tr("Connection:")),      0, 0);
    grid->addWidget(connectionLabel_,                   0, 1);
    grid->addWidget(new QLabel(tr("Mouse firmware:")),  0, 2);
    grid->addWidget(mouseFwLabel_,                      0, 3);
    grid->addWidget(new QLabel(tr("Battery:")),         1, 0);
    grid->addWidget(batteryLabel_,                      1, 1);
    grid->addWidget(new QLabel(tr("Dongle firmware:")), 1, 2);
    grid->addWidget(dongleFwLabel_,                     1, 3);
    grid->addWidget(new QLabel(tr("Signal:")),          2, 0);
    grid->addWidget(signalLabel_,                       2, 1);

    auto* reloadBtn = new QPushButton(tr("Reload"));
    connect(reloadBtn, &QPushButton::clicked, this, &MainWindow::reload);
    grid->addWidget(reloadBtn, 0, 4);

    auto* pairBtn = new QPushButton(tr("Re-pair"));
    pairBtn->setToolTip(tr("Re-establish the radio link between mouse and dongle"));
    connect(pairBtn, &QPushButton::clicked, this, &MainWindow::pairDongle);
    grid->addWidget(pairBtn, 1, 4);

    auto* resetBtn = new QPushButton(tr("Factory Reset"));
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::factoryReset);
    grid->addWidget(resetBtn, 2, 4);

    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);
    return box;
}

QWidget* MainWindow::buildBasicTab()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* form   = new QFormLayout;

    // Filled by repopulateForModel(): the two generations offer different
    // lift-off distances on incompatible scales.
    lodBox_ = new QComboBox;
    form->addRow(tr("Lift-off distance:"), lodBox_);

    cpiLevelsBox_ = new QComboBox;
    for (int i = 1; i <= static_cast<int>(kCpiStageCount); ++i) {
        cpiLevelsBox_->addItem(QString::number(i), i);
    }
    form->addRow(tr("CPI levels:"), cpiLevelsBox_);

    angleSnapBox_  = new QCheckBox(tr("Angle snapping"));
    rippleBox_     = new QCheckBox(tr("Ripple control"));
    ledLiftOffBox_ = new QCheckBox(tr("LED on lift-off"));
    ledLiftOffBox_->setToolTip(
        tr("The underside battery LED also lights when the mouse is lifted. "
           "Unchecking this is the vendor tool's \"Disable LED on Lift-Off\"."));
    splitXYBox_ = new QCheckBox(tr("Separate X / Y"));

    auto* checks = new QHBoxLayout;
    checks->addWidget(angleSnapBox_);
    checks->addWidget(rippleBox_);
    checks->addWidget(ledLiftOffBox_);
    checks->addStretch();

    layout->addLayout(form);
    layout->addLayout(checks);
    layout->addWidget(splitXYBox_);

    auto* cpiBox  = new QGroupBox(tr("CPI stages"));
    auto* cpiGrid = new QGridLayout(cpiBox);
    for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
        // The colour the mouse's underside LED shows for this stage. It is the
        // only way to tell the stages apart on the device itself, and on the v1
        // it is the only way to tell which one is active at all — see
        // repopulateForModel().
        stageSwatch_[i] = new QLabel;
        stageSwatch_[i]->setFixedSize(16, 16);
        applySwatchStyle(stageSwatch_[i], static_cast<size_t>(i), false);
        stageSwatch_[i]->setToolTip(
            tr("The LED under the mouse glows %1 while CPI %2 is the active "
               "stage. The colour belongs to the stage, not to its CPI value.")
                .arg(stageColourName(static_cast<size_t>(i)))
                .arg(i + 1));

        stageButton_[i] = new QPushButton(tr("CPI %1").arg(i + 1));
        stageButton_[i]->setCheckable(true);
        stageButton_[i]->setAutoExclusive(true);
        stageButton_[i]->setToolTip(tr("Make this the active stage"));

        cpiX_[i] = new QSpinBox;
        // Real limits are per model and applied by repopulateForModel(); this
        // is just a safe starting range before the mouse has named itself.
        cpiX_[i]->setRange(kUnknownModel.cpiMin, kUnknownModel.cpiMax);
        cpiX_[i]->setSingleStep(kUnknownModel.cpiStep);
        cpiX_[i]->setSuffix(tr(" CPI"));

        cpiY_[i] = new QSpinBox;
        cpiY_[i]->setRange(kUnknownModel.cpiMin, kUnknownModel.cpiMax);
        cpiY_[i]->setSingleStep(kUnknownModel.cpiStep);
        cpiY_[i]->setSuffix(tr(" CPI"));

        // With split X/Y off, Y follows X.
        connect(cpiX_[i], &QSpinBox::valueChanged, this, [this, i](int v) {
            if (!populating_ && !splitXYBox_->isChecked()) {
                cpiY_[i]->setValue(v);
            }
        });

        cpiGrid->addWidget(stageSwatch_[i],      i, 0);
        cpiGrid->addWidget(stageButton_[i],      i, 1);
        cpiGrid->addWidget(new QLabel(tr("X:")), i, 2);
        cpiGrid->addWidget(cpiX_[i],             i, 3);
        cpiGrid->addWidget(new QLabel(tr("Y:")), i, 4);
        cpiGrid->addWidget(cpiY_[i],             i, 5);
    }

    // Filled in by repopulateForModel(); only the v1 generation needs it.
    cpiStageHint_ = new QLabel;
    cpiStageHint_->setWordWrap(true);
    cpiStageHint_->setVisible(false);
    cpiGrid->addWidget(cpiStageHint_, static_cast<int>(kCpiStageCount), 0, 1, 6);
    connect(splitXYBox_, &QCheckBox::toggled, this, [this](bool on) {
        for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
            cpiY_[i]->setEnabled(on);
            if (!on) {
                cpiY_[i]->setValue(cpiX_[i]->value());
            }
        }
    });
    layout->addWidget(cpiBox);

    auto* apply = new QPushButton(tr("Apply"));
    applySensorBtn_ = apply;
    connect(apply, &QPushButton::clicked, this, &MainWindow::applySensor);
    layout->addStretch();
    layout->addWidget(apply, 0, Qt::AlignRight);
    return page;
}

QWidget* MainWindow::buildAdvancedTab()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);
    auto* form   = new QFormLayout;

    // Filled by repopulateForModel(): only the v2 generation offers the
    // power-saving and office-mode variants.
    pollingBox_ = new QComboBox;
    pollingBox_->setToolTip(
        tr("Wireless power saving is only available at 1000 Hz and below, which "
           "is why 1000 Hz can appear twice. This is separate from the "
           "inactivity timeouts below."));
    form->addRow(tr("Polling rate:"), pollingBox_);

    angleTuningBox_ = new QSpinBox;
    angleTuningBox_->setRange(-30, 30);
    angleTuningBox_->setSuffix(tr(" °"));
    form->addRow(tr("Sensor angle tuning:"), angleTuningBox_);

    motionSyncBox_   = new QCheckBox(tr("Motion sync"));
    forceMaxFpsBox_  = new QCheckBox(tr("Force max sensor FPS"));
    glassModeBox_    = new QCheckBox(tr("Sensor glass mode"));
    // Glass mode also selects which lift-off scale is in force, so moving it
    // has to rebuild that list and translate the stored byte — the two scales
    // do not share a meaning, and carrying the byte over unchanged would mean
    // writing a tenths value to a mouse in glass mode. This mirrors the vendor
    // tool exactly. See PROTOCOL.md section 4.
    connect(glassModeBox_, &QCheckBox::toggled, this, [this](bool on) {
        if (populating_ || !device_.model().hasGlassMode) {
            return;
        }
        // Display only — config_ is NOT touched here. It keeps the values the
        // blob actually holds until an Apply commits them, so a power write
        // that fails cannot leave a converted lift-off byte staged against the
        // old glass bit, waiting for the next Basic-tab Apply to write it.
        const uint8_t shown = lodConvertForGlassMode(config_.sensor.lodIndex, on);
        rebuildLodList(on);
        selectOrAdd(lodBox_, shown,
                    tr("0x%1 (not offered in this mode)")
                        .arg(int(shown), 2, 16, QLatin1Char('0')));
        report(tr("Glass mode changes the lift-off scale; it becomes %1. "
                  "Either Apply writes both settings together.")
                   .arg(lodBox_->currentText()));
    });
    motionJitterBox_ = new QCheckBox(tr("Motion jitter filter"));
    motionJitterBox_->setToolTip(
        tr("Present on the v1 generation only. The v2 models replaced it with "
           "the sensor options beside it."));
    slamclickBox_   = new QCheckBox(tr("Slamclick filter"));
    multiclickBox_  = new QCheckBox(tr("Multiclick filter"));
    multiclickBox_->setToolTip(
        tr("The multiclick filter is not a debounce slider — lower values do "
           "not reduce click latency."));

    auto* toggles = new QHBoxLayout;
    toggles->addWidget(motionSyncBox_);
    toggles->addWidget(forceMaxFpsBox_);
    toggles->addWidget(glassModeBox_);
    toggles->addStretch();

    auto* filters = new QHBoxLayout;
    filters->addWidget(slamclickBox_);
    filters->addWidget(multiclickBox_);
    filters->addWidget(motionJitterBox_);
    filters->addStretch();

    auto* powerRow = new QHBoxLayout;
    powerSavingBox_ = new QCheckBox(tr("Power saving after"));
    powerSavingMin_ = new QSpinBox;
    powerSavingMin_->setRange(kTimeoutMinMinutes, kTimeoutMaxMinutes);
    powerSavingMin_->setSuffix(tr(" min"));
    powerRow->addWidget(powerSavingBox_);
    powerRow->addWidget(powerSavingMin_);
    powerRow->addStretch();
    connect(powerSavingBox_, &QCheckBox::toggled, this, [this](bool on) {
        powerSavingMin_->setEnabled(on);
    });

    auto* sleepRow = new QHBoxLayout;
    deepSleepBox_ = new QCheckBox(tr("Deep sleep after"));
    deepSleepMin_ = new QSpinBox;
    deepSleepMin_->setRange(kTimeoutMinMinutes, kTimeoutMaxMinutes);
    deepSleepMin_->setSuffix(tr(" min"));
    sleepRow->addWidget(deepSleepBox_);
    sleepRow->addWidget(deepSleepMin_);
    sleepRow->addStretch();
    connect(deepSleepBox_, &QCheckBox::toggled, this, [this](bool on) {
        deepSleepMin_->setEnabled(on);
    });

    layout->addLayout(form);
    layout->addLayout(toggles);
    layout->addLayout(filters);
    layout->addLayout(powerRow);
    layout->addLayout(sleepRow);

    auto* filterBox  = new QGroupBox(tr("Click filter"));
    auto* filterForm = new QFormLayout(filterBox);
    for (int i = 0; i < static_cast<int>(kFilterButtonCount); ++i) {
        buttonFilter_[i] = new QComboBox;
        // Unitless on purpose: this is a multiclick count, not a debounce
        // time in milliseconds. The vendor UI shows a bare number too.
        for (int n = 1; n <= 15; ++n) {
            buttonFilter_[i]->addItem(QString::number(n), n);
        }
        // Only the left and right buttons expose the SPDT modes.
        if (buttonHasSpdt(static_cast<size_t>(i))) {
            buttonFilter_[i]->addItem(tr("SPDT — GX Safe Mode"),  0xF0);
            buttonFilter_[i]->addItem(tr("SPDT — GX Speed Mode"), 0xF1);
        }
        filterForm->addRow(tr("%1 button:").arg(buttonName(i)), buttonFilter_[i]);
    }
    layout->addWidget(filterBox);

    auto* apply = new QPushButton(tr("Apply"));
    applyPowerBtn_ = apply;
    connect(apply, &QPushButton::clicked, this, &MainWindow::applyPower);
    layout->addStretch();
    layout->addWidget(apply, 0, Qt::AlignRight);
    return page;
}

QWidget* MainWindow::buildButtonsTab()
{
    auto* page   = new QWidget;
    auto* layout = new QVBoxLayout(page);

    leftHandedBox_ = new QCheckBox(tr("Left-handed mode"));
    leftHandedBox_->setToolTip(tr("Swaps the left and right button actions"));
    layout->addWidget(leftHandedBox_);

    auto* form = new QFormLayout;
    for (int i = 0; i < static_cast<int>(kButtonCount); ++i) {
        buttonAction_[i] = new QComboBox;
        for (const auto& a : kActions) {
            buttonAction_[i]->addItem(QString::fromUtf8(a.label));
        }
        // `activated` rather than `currentIndexChanged`: only a deliberate pick
        // should open the dialog, not a programmatic repopulate.
        connect(buttonAction_[i], &QComboBox::activated, this, [this, i](int idx) {
            if (idx >= 0 && idx < kActionCount &&
                kActions[idx].type == ActionType::CpiPreset) {
                promptFixedCpi(i);
            }
        });
        form->addRow(tr("%1:").arg(buttonName(i)), buttonAction_[i]);
    }
    layout->addLayout(form);

    layout->addWidget(new QLabel(
        tr("“Special” is the CPI button, which the vendor tool does not expose.\n"
           "Keyboard bindings set by the Windows tool are preserved and shown as "
           "“(custom)”; picking another action replaces them.")));

    auto* apply = new QPushButton(tr("Apply"));
    applyButtonsBtn_ = apply;
    connect(apply, &QPushButton::clicked, this, &MainWindow::applyButtons);
    layout->addStretch();
    layout->addWidget(apply, 0, Qt::AlignRight);
    return page;
}

// ----------------------------------------------------------------- model ---

void MainWindow::reload()
{
    setBusy(true);

    // Retry identification first: the mouse may have been asleep at startup,
    // and the unidentified-model message tells the user to press Reload.
    if (!device_.modelIdentified()) {
        device_.identifyModel();
    }

    std::array<uint8_t, kBlobSize> blob{};
    if (!device_.readConfigBlob(blob)) {
        // Nothing was loaded, so the widgets do not describe the device. Keep
        // Apply disabled rather than let it write whatever they happen to hold.
        setConfigLoaded(false);
        setBusy(false);
        report(QString::fromStdString(device_.lastError()), true);
        return;
    }
    config_ = decodeBlob(blob);
    lastReadGlassMode_ = config_.power.glassMode;

    repopulateForModel();
    refreshInfo();
    populate();
    setConfigLoaded(true);
    setBusy(false);

    if (device_.modelIdentified()) {
        report(tr("Configuration loaded."));
    } else {
        report(tr("Configuration loaded, but the mouse is not identified — move "
                  "it to wake it, then press Reload. Lift-off distance and "
                  "polling rate cannot be written until then."), true);
    }
}

void MainWindow::populate()
{
    populating_ = true;

    // A byte the device holds that this model's list does not offer must still
    // round-trip: dropping to index 0 would rewrite it on the next Apply, and
    // an unmatched findData() would leave the box blank and harvest 0.
    selectOrAdd(lodBox_, config_.sensor.lodIndex,
                device_.modelIdentified()
                    ? tr("0x%1 (not offered by this model)")
                          .arg(int(config_.sensor.lodIndex), 2, 16, QLatin1Char('0'))
                    : tr("raw 0x%1 — scale unknown until the mouse is identified")
                          .arg(int(config_.sensor.lodIndex), 2, 16, QLatin1Char('0')));
    selectOrAdd(cpiLevelsBox_, config_.sensor.cpiLevels,
                tr("%1 (unexpected)").arg(config_.sensor.cpiLevels));
    angleSnapBox_->setChecked(config_.sensor.angleSnapping);
    rippleBox_->setChecked(config_.sensor.rippleControl);
    ledLiftOffBox_->setChecked(config_.sensor.ledOnLiftOff);

    bool split = false;
    for (const auto& s : config_.sensor.stages) {
        if (s.x != s.y) {
            split = true;
        }
    }
    splitXYBox_->setChecked(split);
    // The generations allow different CPI ranges — v1 50-26000 step 50, v2
    // 10-30000 step 10 — and while the model is unknown this is kUnknownModel's
    // intersection of the two. Widen around whatever the device actually holds
    // before setValue(), or a value outside the range would be silently clamped
    // and then written back clamped on the next Apply. Same rule as the combos.
    const ModelInfo& model = device_.model();
    for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
        const int x = config_.sensor.stages[i].x;
        const int y = config_.sensor.stages[i].y;
        cpiX_[i]->setRange(std::min<int>(model.cpiMin, x), std::max<int>(model.cpiMax, x));
        cpiY_[i]->setRange(std::min<int>(model.cpiMin, y), std::max<int>(model.cpiMax, y));
        cpiX_[i]->setSingleStep(model.cpiStep);
        cpiY_[i]->setSingleStep(model.cpiStep);
        cpiX_[i]->setValue(x);
        cpiY_[i]->setValue(y);
        cpiY_[i]->setEnabled(split);
    }
    if (config_.sensor.activeStage < kCpiStageCount) {
        stageButton_[config_.sensor.activeStage]->setChecked(true);
    }
    for (size_t i = 0; i < kCpiStageCount; ++i) {
        applySwatchStyle(stageSwatch_[i], i,
                         i == static_cast<size_t>(config_.sensor.activeStage));
    }

    // Same reasoning: a v2 sitting at 0x80 must not be silently rewritten to
    // 4000 Hz just because this model's option list does not include it.
    selectOrAdd(pollingBox_, static_cast<int>(config_.power.pollingMode),
                tr("%1 (not offered by this model)")
                    .arg(QString::fromUtf8(pollingLabel(config_.power.pollingMode))));

    // Same round-trip rule as the combos, applied to a spin box: the vendor
    // offers ±30, but the field is a signed byte, and setValue() on a value
    // outside the range would silently clamp it and write the clamped number
    // back on the next Apply. Widen only as far as the device actually needs.
    // On a model without the field, writeSensorBlock() sends payload +5 as 0
    // (matching the v1 vendor tool, whose serializer has no store for it), so
    // show 0 rather than a number from a blob byte that means something else
    // there and that Apply is about to destroy.
    const int tuning = device_.model().hasAngleTuning ? config_.sensor.angleTuning : 0;
    angleTuningBox_->setRange(std::min(-30, tuning), std::max(30, tuning));
    angleTuningBox_->setValue(tuning);
    motionSyncBox_->setChecked(config_.power.motionSync);
    glassModeBox_->setChecked(config_.power.glassMode);
    forceMaxFpsBox_->setChecked(config_.power.forceMaxFps());
    slamclickBox_->setChecked(config_.power.slamclick());
    multiclickBox_->setChecked(config_.power.multiclick());
    motionJitterBox_->setChecked(config_.power.motionJitter());

    powerSavingBox_->setChecked(config_.power.powerSavingEnabled);
    powerSavingMin_->setValue(config_.power.powerSavingMinutes);
    powerSavingMin_->setEnabled(config_.power.powerSavingEnabled);
    deepSleepBox_->setChecked(config_.power.deepSleepEnabled);
    deepSleepMin_->setValue(config_.power.deepSleepMinutes);
    deepSleepMin_->setEnabled(config_.power.deepSleepEnabled);

    // Same round-trip rule as every other combo here: a value the list does not
    // offer must be kept, not quietly replaced. Falling back to a fixed index
    // rewrote it to 8 ms on the next Apply — and syncFilters() would then push
    // that 8 into the button's 0x16 record too, so the loss was permanent.
    for (int i = 0; i < static_cast<int>(kFilterButtonCount); ++i) {
        selectOrAdd(buttonFilter_[i], config_.power.buttonFilter[i],
                    tr("0x%1 (unexpected)")
                        .arg(int(config_.power.buttonFilter[i]), 2, 16, QLatin1Char('0')));
    }

    leftHandedBox_->setChecked(config_.buttons.isLeftHanded());

    for (int i = 0; i < static_cast<int>(kButtonCount); ++i) {
        const int found = indexOfAction(config_.buttons.entries[i]);
        if (found >= 0) {
            if (buttonAction_[i]->count() > kActionCount) {
                buttonAction_[i]->removeItem(buttonAction_[i]->count() - 1);
            }
            buttonAction_[i]->setCurrentIndex(found);
        } else {
            // Preserve whatever is on the device; offer it as a custom entry.
            const QString label =
                tr("(custom) %1")
                    .arg(QString::fromStdString(config_.buttons.entries[i].describe()));
            if (buttonAction_[i]->count() == kActionCount) {
                buttonAction_[i]->addItem(label);
            } else {
                buttonAction_[i]->setItemText(buttonAction_[i]->count() - 1, label);
            }
            buttonAction_[i]->setCurrentIndex(buttonAction_[i]->count() - 1);
        }
    }

    populating_ = false;
}

void MainWindow::harvestSensor()
{
    config_.sensor.lodIndex      = static_cast<uint8_t>(lodBox_->currentData().toInt());
    config_.sensor.cpiLevels     = static_cast<uint8_t>(cpiLevelsBox_->currentData().toInt());
    config_.sensor.angleSnapping = angleSnapBox_->isChecked();
    config_.sensor.rippleControl = rippleBox_->isChecked();
    config_.sensor.ledOnLiftOff  = ledLiftOffBox_->isChecked();
    config_.sensor.angleTuning   = static_cast<int8_t>(angleTuningBox_->value());

    // The "Separate X / Y" checkbox drives nothing but the Y spin boxes: the
    // per-stage xySplit flag is derived from x != y inside SensorBlock::encode,
    // so there is nothing to harvest for it here.
    for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
        config_.sensor.stages[i].x = static_cast<uint16_t>(cpiX_[i]->value());
        config_.sensor.stages[i].y = static_cast<uint16_t>(cpiY_[i]->value());
        // Only on a model that implements the field. Writes are whole-block, so
        // there is no way to omit payload +7; leaving config_ untouched sends
        // back exactly the byte the blob read, which is what the v1 vendor tool
        // does too. See ModelInfo::hasCpiStageSelect.
        if (device_.model().hasCpiStageSelect && stageButton_[i]->isChecked()) {
            config_.sensor.activeStage = static_cast<uint8_t>(i);
        }
    }
}

void MainWindow::harvestPower()
{
    config_.power.pollingMode = static_cast<uint8_t>(pollingBox_->currentData().toInt());
    config_.power.motionSync  = motionSyncBox_->isChecked();
    config_.power.glassMode   = glassModeBox_->isChecked();
    // Only touch bits this model actually implements, so a flag belonging to
    // the other generation is preserved rather than cleared.
    const ModelInfo& m = device_.model();
    config_.power.setFlag(kSlamclickFilter, slamclickBox_->isChecked());
    if (m.hasForceMaxFps) {
        config_.power.setFlag(kForceMaxSensorFps, forceMaxFpsBox_->isChecked());
    }
    if (m.hasMulticlickAck) {
        config_.power.setFlag(kMulticlickFilter, multiclickBox_->isChecked());
    }
    if (m.hasMotionJitter) {
        config_.power.setFlag(kMotionJitterFilter, motionJitterBox_->isChecked());
    }

    config_.power.powerSavingEnabled = powerSavingBox_->isChecked();
    config_.power.powerSavingMinutes = static_cast<uint8_t>(powerSavingMin_->value());
    config_.power.deepSleepEnabled   = deepSleepBox_->isChecked();
    config_.power.deepSleepMinutes   = static_cast<uint8_t>(deepSleepMin_->value());

    for (int i = 0; i < static_cast<int>(kFilterButtonCount); ++i) {
        config_.power.buttonFilter[i] =
            static_cast<uint8_t>(buttonFilter_[i]->currentData().toInt());
    }
}

void MainWindow::harvestButtons()
{
    for (int i = 0; i < static_cast<int>(kButtonCount); ++i) {
        const int idx = buttonAction_[i]->currentIndex();
        if (idx < 0 || idx >= kActionCount) {
            continue;   // the "(custom)" row — leave the device value alone
        }
        const Action& a = kActions[idx];
        if (a.type == ActionType::CpiPreset) {
            continue;   // promptFixedCpi() already wrote the model
        }
        // Clear the whole payload first: switching away from a Fixed CPI
        // binding must not leave its CPI bytes behind.
        config_.buttons.entries[i].clearPayload();
        config_.buttons.entries[i].type = static_cast<uint8_t>(a.type);
        config_.buttons.entries[i].code = a.code;
    }
    // Applied after the per-button actions, so it swaps whatever was just set.
    config_.buttons.setLeftHanded(leftHandedBox_->isChecked());
}

// ---------------------------------------------------------------- actions ---

void MainWindow::applySensor()
{
    harvestSensor();
    uint8_t buf[kSensorPayload];
    config_.sensor.encode(buf);

    setBusy(true);
    bool ok = device_.writeSensorBlock(buf);

    // The lift-off byte just written is on the scale the glass checkbox shows.
    // If that checkbox has moved since the blob was read, the device still has
    // the old glass bit, so the pair would be split down the middle. Write the
    // power block too — only the glass bit comes from the widgets, the rest of
    // the block is config_ as read, so this cannot commit unapplied edits from
    // the Advanced tab.
    if (ok && glassPending()) {
        config_.power.glassMode = glassModeBox_->isChecked();
        uint8_t powerBuf[kPowerPayload];
        config_.power.encode(powerBuf);
        ok = device_.writePowerBlock(powerBuf);
        if (ok) {
            lastReadGlassMode_ = config_.power.glassMode;
        }
    }
    setBusy(false);

    report(ok ? tr("Sensor settings applied.")
              : QString::fromStdString(device_.lastError()), !ok);
}

void MainWindow::applyPower()
{
    harvestPower();
    uint8_t buf[kPowerPayload];
    config_.power.encode(buf);

    setBusy(true);
    bool ok = device_.writePowerBlock(buf);

    // Sensor angle tuning sits on this tab because that is where the vendor
    // tool puts it — but it is a cmd 0x14 field, so this Apply has to write
    // that block too or the spinbox would silently do nothing. Only that one
    // value is taken from the widgets; the rest of the block comes from
    // config_ as it was read, so applying here cannot quietly commit
    // unapplied Basic-tab edits. v1 models do not have the field at all.
    // The two blocks are coupled only when glass mode has actually MOVED: the
    // glass bit in 0x15 decides what the lift-off byte in 0x14 means. Writing
    // 0x14 on every Advanced-tab Apply would restate the active CPI stage and
    // the four CPI records from a possibly stale config_, silently reverting a
    // stage the user had changed with the button under the mouse.
    bool needSensor = false;
    if (glassPending()) {
        // Take the byte the user has been shown, which is what rebuildLodList()
        // and the toggle handler put in the combo.
        config_.sensor.lodIndex = static_cast<uint8_t>(lodBox_->currentData().toInt());
        needSensor = true;
    }

    if (device_.model().hasAngleTuning) {
        const auto tuning = static_cast<int8_t>(angleTuningBox_->value());
        if (tuning != config_.sensor.angleTuning) {
            config_.sensor.angleTuning = tuning;
            needSensor = true;
        }
    }

    // Only angle tuning and the converted lift-off byte come from this tab;
    // everything else in the block is config_ as loaded, so applying here
    // cannot quietly commit unapplied Basic-tab edits.
    if (ok && needSensor) {
        uint8_t sensorBuf[kSensorPayload];
        config_.sensor.encode(sensorBuf);
        ok = device_.writeSensorBlock(sensorBuf);
    }
    setBusy(false);

    if (ok) {
        lastReadGlassMode_ = config_.power.glassMode;
        // The filter values also live in the button records.
        syncFilters(config_.power, config_.buttons);
    }
    report(ok ? tr("Advanced settings applied.")
              : QString::fromStdString(device_.lastError()), !ok);
}

void MainWindow::applyButtons()
{
    harvestButtons();
    syncFilters(config_.power, config_.buttons);

    uint8_t buf[kButtonTableBytes];
    config_.buttons.encode(buf);

    setBusy(true);
    const bool ok = device_.writeButtonTable(buf);
    setBusy(false);

    if (ok) {
        populate();   // left-handed may have reordered the rows
    }
    report(ok ? tr("Button mapping applied.")
              : QString::fromStdString(device_.lastError()), !ok);
}

void MainWindow::refreshInfo()
{
    // The dongle answers over USB whether or not the mouse is awake, so its
    // firmware stays readable while everything mouse-side goes blank — the
    // same split the vendor tool shows.
    if (auto v = device_.dongleFirmware()) {
        dongleFwLabel_->setText(QString::fromStdString(v->toString()));
    }

    if (auto v = device_.mouseFirmware()) {
        mouseFwLabel_->setText(QString::fromStdString(v->toString()));
        connectionLabel_->setText(device_.info().wired ? tr("Wired") : tr("Wireless"));
        if (auto s = device_.batteryStatus()) {
            batteryLabel_->setText(QString("%1 %").arg(s->percent));
            signalLabel_->setText(QString::number(s->signal));
        } else {
            batteryLabel_->setText(tr("—"));
            signalLabel_->setText(tr("—"));
        }
    } else {
        showAsleep();
    }
}

void MainWindow::promptFixedCpi(int buttonIndex)
{
    ButtonEntry& e = config_.buttons.entries[buttonIndex];
    const int curX = e.isFixedCpi() ? e.fixedCpiX() : 800;
    const int curY = e.isFixedCpi() ? e.fixedCpiY() : curX;

    bool ok = false;
    const int x = QInputDialog::getInt(
        this, tr("Fixed CPI"), tr("%1 — CPI (X):").arg(buttonName(buttonIndex)),
        curX, device_.model().cpiMin, device_.model().cpiMax,
        device_.model().cpiStep, &ok);
    if (!ok) {
        populate();   // restore the combo to what the device actually has
        return;
    }

    int y = x;
    if (splitXYBox_->isChecked()) {
        y = QInputDialog::getInt(
            this, tr("Fixed CPI"), tr("%1 — CPI (Y):").arg(buttonName(buttonIndex)),
            curY, device_.model().cpiMin, device_.model().cpiMax,
            device_.model().cpiStep, &ok);
        if (!ok) {
            populate();
            return;
        }
    }

    e.setFixedCpi(static_cast<uint16_t>(x), static_cast<uint16_t>(y));
    populate();   // re-render the row through describe()
    report(tr("%1 set to %2 — press Apply to write it.")
               .arg(buttonName(buttonIndex), QString::fromStdString(e.describe())));
}

// The lift-off list depends on the model AND on sensor glass mode, so it is
// rebuilt both on reload and whenever that checkbox moves.
bool MainWindow::glassPending() const
{
    return device_.model().hasGlassMode &&
           glassModeBox_->isChecked() != lastReadGlassMode_;
}

void MainWindow::rebuildLodList(bool glassMode)
{
    const ModelInfo& m = device_.model();

    lodBox_->clear();
    if (!device_.modelIdentified()) {
        // kUnknownModel carries the v2 encoding as a placeholder, and building
        // the list from it would render a sleeping v1's byte 1 as "0.8 mm" — a
        // wrong number, confidently displayed. The CLI prints the raw byte
        // here; do the same rather than disagree with it. populate() fills in
        // the raw byte through selectOrAdd(), and the box is disabled.
        return;
    }

    // Passed in, never read off the checkbox here: repopulateForModel() runs
    // before populate(), so at that point the widget still holds the previous
    // device's state.
    const LodEncoding enc = effectiveLodEncoding(m, glassMode);

    // Whole millimetres are labelled "1 mm" / "2 mm": a decimal would imply a
    // precision that scale does not have, and the vendor writes "1mm" too.
    const int decimals = (enc == LodEncoding::Millimetres) ? 0 : 1;
    for (double mm : lodOptions(enc)) {
        lodBox_->addItem(QString::number(mm, 'f', decimals) + tr(" mm"),
                         lodMillimetresToIndex(mm, enc));
    }
}

void MainWindow::repopulateForModel()
{
    const ModelInfo& m = device_.model();
    const bool known = device_.modelIdentified();

    // Qt renders this as "<title> — Endgame Gear" (the application display
    // name), so this half carries only the model.
    setWindowTitle(known ? QString::fromUtf8(m.name)
                         : tr("Mouse not identified"));

    // Lift-off distance: v1 offers 1 and 2 mm; v2 offers 0.7–2.0 mm in 0.1 mm
    // steps, and the byte means different things on each. Rebuild rather than
    // filter, and do not try to carry the old selection across — the rows mean
    // something different afterwards. populate() sets the selection from the
    // device immediately after.
    rebuildLodList(config_.power.glassMode);

    pollingBox_->clear();
    for (const auto& o : pollingOptions(m)) {
        pollingBox_->addItem(QString::fromUtf8(o.label),
                             static_cast<int>(static_cast<uint8_t>(o.mode)));
    }

    // The v1 firmware ignores cmd 0x14 +7, so offering the buttons would be a
    // lie — the user clicks, applies, and nothing moves. Its own vendor tool
    // has no such control either. The swatches still show which stage is
    // active, which is the part that was actually missing.
    const bool canSelectStage = known && m.hasCpiStageSelect;
    const QString stageTip =
        canSelectStage ? tr("Make this the active stage")
      : known          ? tr("This model does not switch stages from software.")
                       : tr("Waiting for the mouse to say which model it is.");
    for (auto* b : stageButton_) {
        b->setEnabled(canSelectStage);
        b->setToolTip(stageTip);
    }
    cpiStageHint_->setVisible(known && !m.hasCpiStageSelect);
    cpiStageHint_->setText(
        tr("This model switches CPI stages with the button underneath the "
           "mouse, not from software. The highlighted swatch is the stage the "
           "mouse is on now; its colour is what the LED shows.\n"
           "Any button can be given the same job: Button Mapping → CPI cycle."));

    // A greyed control with no explanation is indistinguishable from one that
    // has simply not loaded yet, so say which it is. The tooltip each control
    // was built with is kept and restored when the model does support it.
    const auto gate = [&](QWidget* w, bool supported) {
        if (!w->property("baseTip").isValid()) {
            w->setProperty("baseTip", w->toolTip());
        }
        w->setEnabled(known && supported);
        w->setToolTip(
            !known       ? tr("Waiting for the mouse to say which model it is.")
          : !supported   ? tr("Not present on the %1.").arg(QString::fromUtf8(m.name))
                         : w->property("baseTip").toString());
    };
    gate(angleTuningBox_,  m.hasAngleTuning);
    gate(glassModeBox_,    m.hasGlassMode);
    gate(forceMaxFpsBox_,  m.hasForceMaxFps);
    gate(multiclickBox_,   m.hasMulticlickAck);
    gate(motionJitterBox_, m.hasMotionJitter);

    // Until cmd 0x0E names the mouse we do not know which lift-off scale or
    // polling set applies. The device layer refuses those writes outright;
    // greying the inputs just makes that visible before the user tries.
    lodBox_->setEnabled(known);
    pollingBox_->setEnabled(known);
}

// The widgets only describe the device once a config blob has been read.
// Before that — or after a failed reload — Apply would write whatever they
// happen to hold, so it stays disabled.
void MainWindow::setConfigLoaded(bool loaded)
{
    if (applySensorBtn_)  applySensorBtn_->setEnabled(loaded);
    if (applyPowerBtn_)   applyPowerBtn_->setEnabled(loaded);
    if (applyButtonsBtn_) applyButtonsBtn_->setEnabled(loaded);
}

// Selects `value` in `box`, adding a row for it if the model's own list does
// not contain it. That keeps a byte the device already holds round-tripping
// unchanged instead of silently collapsing to the first entry.
void MainWindow::selectOrAdd(QComboBox* box, int value, const QString& fallbackLabel)
{
    int i = box->findData(value);
    if (i < 0) {
        box->addItem(fallbackLabel, value);
        i = box->count() - 1;
    }
    box->setCurrentIndex(i);
}

void MainWindow::showAsleep()
{
    mouseFwLabel_->setText(tr("—"));
    batteryLabel_->setText(tr("—"));
    signalLabel_->setText(tr("—"));
    connectionLabel_->setText(tr("Asleep"));
}

void MainWindow::pollEvents()
{
    // Everything here comes from the dongle unprompted — no USB traffic is
    // generated unless a link-up event tells us there is fresh state to read.
    while (auto ev = device_.pollEvent()) {
        if (ev->isBattery()) {
            batteryLabel_->setText(QString("%1 %").arg(ev->batteryPercent()));
            signalLabel_->setText(QString::number(ev->signalLevel()));
            connectionLabel_->setText(device_.info().wired ? tr("Wired") : tr("Wireless"));
            mouseFwLabel_->setEnabled(true);
        } else if (ev->isPollingChanged()) {
            // The mouse can change its own polling rate; the v1 vendor tool
            // follows this event into its combo box. Accept only a value this
            // model offers, so a misread cannot leave a wrong byte staged for
            // the next Apply.
            const int mode  = ev->pollingModeByte();
            const int index = pollingBox_->findData(mode);
            if (index >= 0) {
                config_.power.pollingMode = static_cast<uint8_t>(mode);
                pollingBox_->setCurrentIndex(index);
                report(tr("Polling rate changed on the mouse: %1.")
                           .arg(QString::fromUtf8(
                               pollingLabel(config_.power.pollingMode))));
            } else {
                report(tr("Device reported a polling rate this model does not "
                          "list: %1").arg(QString::fromStdString(ev->toHex())));
            }
        } else if (ev->isLinkState() && (ev->linkUp() || ev->linkDown())) {
            if (ev->linkUp()) {
                report(tr("Mouse woke up."));
                // If it was asleep at startup we could not identify it. Now we
                // can — and a full reload is needed, not just a repopulate:
                // the widgets must be refilled from a freshly read blob, or
                // the next Apply would write stale defaults.
                if (!device_.modelIdentified() && device_.identifyModel()) {
                    reload();
                    continue;
                }
                refreshInfo();
            } else {
                showAsleep();
                report(tr("Mouse went to sleep."));
            }
        } else {
            // A code no vendor tool acts on. Theirs drop these; this one says
            // so, because an undecoded event is worth knowing about — but it
            // is harmless, and in particular it does not mean the Apply that
            // may have preceded it failed. At least two exist: a v1 emits 0x31
            // after a cmd 0x14 write, and 0x30 appears in four captures. See
            // PROTOCOL.md section 4a.
            report(tr("Device event %1, not decoded (harmless): %2")
                       .arg(int(ev->code()), 2, 16, QLatin1Char('0'))
                       .arg(QString::fromStdString(ev->toHex())));
        }
    }
}

void MainWindow::pairDongle()
{
    const auto choice = QMessageBox::question(
        this, tr("Re-pair"),
        tr("Re-establish the radio link between the mouse and the dongle?\n\n"
           "The mouse will disconnect briefly and come back. Settings are not "
           "affected."),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);

    if (choice != QMessageBox::Ok) {
        return;
    }

    setBusy(true);
    const bool ok = device_.pair();
    setBusy(false);

    report(ok ? tr("Pairing started — the mouse will reconnect shortly.")
              : QString::fromStdString(device_.lastError()), !ok);
}

void MainWindow::factoryReset()
{
    const auto choice = QMessageBox::warning(
        this, tr("Factory reset"),
        tr("Reset the mouse to factory defaults?\n\n"
           "All CPI stages, button mappings and sensor settings will be lost."),
        QMessageBox::Reset | QMessageBox::Cancel, QMessageBox::Cancel);

    if (choice != QMessageBox::Reset) {
        return;
    }

    setBusy(true);
    const bool ok = device_.factoryReset();
    setBusy(false);

    if (ok) {
        report(tr("Factory reset sent."));
        reload();
    } else {
        report(QString::fromStdString(device_.lastError()), true);
    }
}

void MainWindow::setBusy(bool busy)
{
    if (busy) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
    QApplication::processEvents();
}

void MainWindow::report(const QString& message, bool error)
{
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet(error ? "color: #b00020;" : QString());
}
