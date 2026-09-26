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

}  // namespace

MainWindow::MainWindow()
{
    // Neutral until the mouse names itself; repopulateForModel() replaces this
    // with the actual model. Hardcoding a model here would mislabel the other.
    setWindowTitle(tr("Endgame Gear"));

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
        stageButton_[i] = new QPushButton(tr("CPI %1").arg(i + 1));
        stageButton_[i]->setCheckable(true);
        stageButton_[i]->setAutoExclusive(true);
        stageButton_[i]->setToolTip(tr("Make this the active stage"));

        cpiX_[i] = new QSpinBox;
        cpiX_[i]->setRange(50, 26000);
        cpiX_[i]->setSingleStep(50);
        cpiX_[i]->setSuffix(tr(" CPI"));

        cpiY_[i] = new QSpinBox;
        cpiY_[i]->setRange(50, 26000);
        cpiY_[i]->setSingleStep(50);
        cpiY_[i]->setSuffix(tr(" CPI"));

        // With split X/Y off, Y follows X.
        connect(cpiX_[i], &QSpinBox::valueChanged, this, [this, i](int v) {
            if (!populating_ && !splitXYBox_->isChecked()) {
                cpiY_[i]->setValue(v);
            }
        });

        cpiGrid->addWidget(stageButton_[i],      i, 0);
        cpiGrid->addWidget(new QLabel(tr("X:")), i, 1);
        cpiGrid->addWidget(cpiX_[i],             i, 2);
        cpiGrid->addWidget(new QLabel(tr("Y:")), i, 3);
        cpiGrid->addWidget(cpiY_[i],             i, 4);
    }
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
                tr("0x%1 (not offered by this model)")
                    .arg(config_.sensor.lodIndex, 2, 16, QLatin1Char('0')));
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
    for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
        cpiX_[i]->setValue(config_.sensor.stages[i].x);
        cpiY_[i]->setValue(config_.sensor.stages[i].y);
        cpiY_[i]->setEnabled(split);
    }
    if (config_.sensor.activeStage < kCpiStageCount) {
        stageButton_[config_.sensor.activeStage]->setChecked(true);
    }

    // Same reasoning: a v2 sitting at 0x80 must not be silently rewritten to
    // 4000 Hz just because this model's option list does not include it.
    selectOrAdd(pollingBox_, static_cast<int>(config_.power.pollingMode),
                tr("%1 (not offered by this model)")
                    .arg(QString::fromUtf8(pollingLabel(config_.power.pollingMode))));

    angleTuningBox_->setValue(config_.sensor.angleTuning);
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

    for (int i = 0; i < static_cast<int>(kFilterButtonCount); ++i) {
        const int found = buttonFilter_[i]->findData(config_.power.buttonFilter[i]);
        buttonFilter_[i]->setCurrentIndex(found >= 0 ? found : 7);   // fall back to 8 ms
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

    const bool split = splitXYBox_->isChecked();
    for (int i = 0; i < static_cast<int>(kCpiStageCount); ++i) {
        config_.sensor.stages[i].x = static_cast<uint16_t>(cpiX_[i]->value());
        config_.sensor.stages[i].y = static_cast<uint16_t>(cpiY_[i]->value());
        // Written only when the user asks for separate axes, so a stage that
        // was combined stays combined.
        config_.sensor.stages[i].xySplit = split ? 1 : 0;
        if (stageButton_[i]->isChecked()) {
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
    const bool ok = device_.writeSensorBlock(buf);
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
    const bool ok = device_.writePowerBlock(buf);
    setBusy(false);

    if (ok) {
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
        curX, 50, 26000, 50, &ok);
    if (!ok) {
        populate();   // restore the combo to what the device actually has
        return;
    }

    int y = x;
    if (splitXYBox_->isChecked()) {
        y = QInputDialog::getInt(
            this, tr("Fixed CPI"), tr("%1 — CPI (Y):").arg(buttonName(buttonIndex)),
            curY, 50, 26000, 50, &ok);
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

void MainWindow::repopulateForModel()
{
    const ModelInfo& m = device_.model();
    const bool known = device_.modelIdentified();

    setWindowTitle(known ? QString::fromUtf8(m.name)
                         : tr("Endgame Gear — mouse not identified"));

    // Lift-off distance: v1 offers 1 and 2 mm; v2 offers 0.7–2.0 mm in 0.1 mm
    // steps, and the byte means different things on each. Rebuild rather than
    // filter, and do not try to carry the old selection across — the rows mean
    // something different afterwards. populate() sets the selection from the
    // device immediately after.
    lodBox_->clear();
    for (double mm : lodOptions(m.lod)) {
        lodBox_->addItem(QString::number(mm, 'f', 1) + tr(" mm"),
                         lodMillimetresToIndex(mm, m.lod));
    }

    pollingBox_->clear();
    for (const auto& o : pollingOptions(m)) {
        pollingBox_->addItem(QString::fromUtf8(o.label),
                             static_cast<int>(static_cast<uint8_t>(o.mode)));
    }

    angleTuningBox_->setEnabled(m.hasAngleTuning);
    glassModeBox_->setEnabled(m.hasGlassMode);
    forceMaxFpsBox_->setEnabled(m.hasForceMaxFps);
    multiclickBox_->setEnabled(m.hasMulticlickAck);
    motionJitterBox_->setEnabled(m.hasMotionJitter);

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
        } else if (ev->isLinkState()) {
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
            // Unrecognised event code; surface it rather than swallowing it.
            report(tr("Unknown device event: %1")
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
