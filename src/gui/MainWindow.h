#pragma once

#include <QMainWindow>
#include <array>

#include "vole/device.h"
#include "vole/settings.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow();

private slots:
    void reload();
    void applySensor();
    void applyPower();
    void applyButtons();
    void refreshInfo();
    void pollEvents();
    void factoryReset();
    void pairDongle();

private:
    QWidget* buildInfoPanel();
    QWidget* buildBasicTab();
    QWidget* buildAdvancedTab();
    QWidget* buildButtonsTab();

    void populate();                       // model -> widgets
    void harvestSensor();                  // widgets -> model
    void harvestPower();
    void harvestButtons();
    void showAsleep();
    void repopulateForModel();
    void rebuildLodList(bool glassMode);

    // True when the glass-mode checkbox has moved since the blob was read, so
    // the lift-off byte on screen is on a different scale from the one the
    // device is using. Both Apply buttons close that gap.
    bool glassPending() const;
    void setConfigLoaded(bool loaded);
    static void selectOrAdd(QComboBox* box, int value, const QString& fallbackLabel);
    void promptFixedCpi(int buttonIndex);
    void setBusy(bool busy);
    void report(const QString& message, bool error = false);



    vole::Device        device_;
    vole::DecodedConfig config_;
    bool               populating_ = false;
    // The glass bit the last successful read/write left on the device. Not
    // config_.power.glassMode, which tracks the widgets.
    bool               lastReadGlassMode_ = false;

    // info
    QLabel* connectionLabel_ = nullptr;
    QLabel* batteryLabel_    = nullptr;
    QLabel* mouseFwLabel_    = nullptr;
    QLabel* dongleFwLabel_   = nullptr;
    QLabel* statusLabel_     = nullptr;

    // basic
    QComboBox* lodBox_        = nullptr;
    QComboBox* cpiLevelsBox_  = nullptr;
    QCheckBox* angleSnapBox_  = nullptr;
    QCheckBox* rippleBox_     = nullptr;
    QCheckBox* ledLiftOffBox_ = nullptr;
    QCheckBox* splitXYBox_    = nullptr;
    std::array<QSpinBox*, vole::kCpiStageCount>    cpiX_{};
    std::array<QSpinBox*, vole::kCpiStageCount>    cpiY_{};
    std::array<QPushButton*, vole::kCpiStageCount> stageButton_{};
    std::array<QLabel*, vole::kCpiStageCount>      stageSwatch_{};
    QLabel* cpiStageHint_ = nullptr;

    // advanced
    QComboBox* pollingBox_     = nullptr;
    QCheckBox* motionSyncBox_  = nullptr;
    QCheckBox* glassModeBox_   = nullptr;
    QCheckBox* forceMaxFpsBox_ = nullptr;
    QCheckBox* motionJitterBox_ = nullptr;
    QSpinBox*  angleTuningBox_ = nullptr;
    QCheckBox* slamclickBox_   = nullptr;
    QCheckBox* multiclickBox_  = nullptr;
    QCheckBox* powerSavingBox_ = nullptr;
    QSpinBox*  powerSavingMin_ = nullptr;
    QCheckBox* deepSleepBox_   = nullptr;
    QSpinBox*  deepSleepMin_   = nullptr;
    std::array<QComboBox*, vole::kFilterButtonCount> buttonFilter_{};

    // buttons
    QCheckBox* leftHandedBox_ = nullptr;
    std::array<QComboBox*, vole::kButtonCount> buttonAction_{};

    QPushButton* applySensorBtn_  = nullptr;
    QPushButton* applyPowerBtn_   = nullptr;
    QPushButton* applyButtonsBtn_ = nullptr;

    QTimer* eventTimer_ = nullptr;
};
