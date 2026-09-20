#pragma once

#include <QMainWindow>
#include <array>

#include "egg/device.h"
#include "egg/settings.h"

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
    void promptFixedCpi(int buttonIndex);
    void setBusy(bool busy);
    void report(const QString& message, bool error = false);



    egg::Device        device_;
    egg::DecodedConfig config_;
    bool               populating_ = false;

    // info
    QLabel* connectionLabel_ = nullptr;
    QLabel* batteryLabel_    = nullptr;
    QLabel* signalLabel_     = nullptr;
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
    std::array<QSpinBox*, egg::kCpiStageCount>    cpiX_{};
    std::array<QSpinBox*, egg::kCpiStageCount>    cpiY_{};
    std::array<QPushButton*, egg::kCpiStageCount> stageButton_{};

    // advanced
    QComboBox* pollingBox_     = nullptr;
    QCheckBox* motionSyncBox_  = nullptr;
    QCheckBox* glassModeBox_   = nullptr;
    QCheckBox* forceMaxFpsBox_ = nullptr;
    QSpinBox*  angleTuningBox_ = nullptr;
    QCheckBox* slamclickBox_   = nullptr;
    QCheckBox* multiclickBox_  = nullptr;
    QCheckBox* powerSavingBox_ = nullptr;
    QSpinBox*  powerSavingMin_ = nullptr;
    QCheckBox* deepSleepBox_   = nullptr;
    QSpinBox*  deepSleepMin_   = nullptr;
    std::array<QComboBox*, egg::kFilterButtonCount> buttonFilter_{};

    // buttons
    QCheckBox* leftHandedBox_ = nullptr;
    std::array<QComboBox*, egg::kButtonCount> buttonAction_{};

    QTimer* eventTimer_ = nullptr;
};
