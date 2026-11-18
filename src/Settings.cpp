// Zadabar64 - C64 SID instrument
// Copyright (C) 2026 Zadabar
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.h"

namespace zadabar
{

Settings::Settings()
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "Zadabar64";
    o.folderName          = "Zadabar";
    o.filenameSuffix      = ".settings";
    o.osxLibrarySubFolder = "Application Support";
    o.storageFormat       = juce::PropertiesFile::storeAsXML;
    props.setStorageParameters (o);
}

int Settings::getSkin() const
{
    if (auto* f = const_cast<juce::ApplicationProperties&> (props).getUserSettings())
        return juce::jlimit (0, 2, f->getIntValue ("skin", 0));
    return 0;
}

void Settings::setSkin (int skinIndex)
{
    if (auto* f = props.getUserSettings())
    {
        f->setValue ("skin", skinIndex);
        f->saveIfNeeded();
    }
    sendChangeMessage();
}

int Settings::getTiming() const
{
    if (auto* f = const_cast<juce::ApplicationProperties&> (props).getUserSettings())
        return juce::jlimit (0, 1, f->getIntValue ("timing", 0));
    return 0;
}

void Settings::setTiming (int timing)
{
    if (auto* f = props.getUserSettings())
    {
        f->setValue ("timing", timing);
        f->saveIfNeeded();
    }
    sendChangeMessage();
}

} // namespace zadabar
