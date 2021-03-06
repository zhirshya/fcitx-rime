//
// Copyright (C) 2018~2018 by xuzhao9 <i@xuzhao.net>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING. If not,
// see <http://www.gnu.org/licenses/>.
//
#include "Model.h"
#include "Common.h"
#include "keynametable.h"
#include <algorithm>
#include <fcitxqtkeysequencewidget.h>
#include <iostream>

namespace fcitx_rime {
void RimeConfigDataModel::sortSchemas() {
    std::sort(schemas_.begin(), schemas_.end(),
              [](const FcitxRimeSchema &a, const FcitxRimeSchema &b) -> bool {
                  // if both inactive, sort by id
                  if (a.index == 0 && b.index == 0) {
                      return a.id < b.id;
                  }
                  if (a.index == 0) {
                      return false;
                  }
                  if (b.index == 0) {
                      return true;
                  }
                  return (a.index < b.index);
              });
}

void RimeConfigDataModel::sortKeys() {
    sortSingleKeySet(toggle_keys);
    sortSingleKeySet(ascii_key);
    sortSingleKeySet(trasim_key);
    sortSingleKeySet(halffull_key);
    sortSingleKeySet(pgup_key);
    sortSingleKeySet(pgdown_key);
}

void RimeConfigDataModel::sortSingleKeySet(QVector<FcitxKeySeq> &keys) {
    std::sort(keys.begin(), keys.end(),
              [](const FcitxKeySeq &a, const FcitxKeySeq &b) -> bool {
                  auto qa = QKeySequence(FcitxQtKeySequenceWidget::keyFcitxToQt(
                      a.sym_, a.states_));
                  auto qb = QKeySequence(FcitxQtKeySequenceWidget::keyFcitxToQt(
                      b.sym_, b.states_));
                  return qa.toString().length() < qb.toString().length();
              });
}

void RimeConfigDataModel::setKeybindings(std::vector<Keybinding> bindings) {
    for (const auto &binding : bindings) {
        if (binding.accept.empty()) {
            continue;
        }
        if (binding.action == "ascii_mode") {
            FcitxKeySeq seq(binding.accept);
            ascii_key.push_back(seq);
        } else if (binding.action == "full_shape") {
            FcitxKeySeq seq(binding.accept);
            halffull_key.push_back(seq);
        } else if (binding.action == "simplification") {
            FcitxKeySeq seq(binding.accept);
            trasim_key.push_back(seq);
        } else if (binding.action == "Page_Up") {
            FcitxKeySeq seq(binding.accept);
            pgup_key.push_back(seq);
        } else if (binding.action == "Page_Down") {
            FcitxKeySeq seq(binding.accept);
            pgdown_key.push_back(seq);
        }
    }
    sortKeys();
}

std::vector<Keybinding> RimeConfigDataModel::getKeybindings() {
    std::vector<Keybinding> out;
    // Fill ascii_key
    for (auto &ascii : ascii_key) {
        Keybinding binding;
        binding.action = "ascii_mode";
        binding.when = KeybindingCondition::Always;
        binding.type = KeybindingType::Toggle;
        binding.accept = ascii.toString();
        out.push_back(binding);
    }
    // Fill trasim_key
    for (auto &trasim : trasim_key) {
        Keybinding binding;
        binding.action = "simplification";
        binding.when = KeybindingCondition::Always;
        binding.type = KeybindingType::Toggle;
        binding.accept = trasim.toString();
        out.push_back(binding);
    }
    // Fill halffull_key
    for (auto &halffull : halffull_key) {
        Keybinding binding;
        binding.action = "full_shape";
        binding.when = KeybindingCondition::Always;
        binding.type = KeybindingType::Toggle;
        binding.accept = halffull.toString();
        out.push_back(binding);
    }
    // Fill pgup_key
    for (auto &pgup : pgup_key) {
        Keybinding binding;
        binding.action = "Page_Up";
        binding.when = KeybindingCondition::HasMenu;
        binding.type = KeybindingType::Send;
        binding.accept = pgup.toString();
        out.push_back(binding);
    }
    // Fill pgdown_key
    for (auto &pgup : pgdown_key) {
        Keybinding binding;
        binding.action = "Page_Down";
        binding.when = KeybindingCondition::HasMenu;
        binding.type = KeybindingType::Send;
        binding.accept = pgup.toString();
        out.push_back(binding);
    }
    return out;
}

// default constructor
FcitxKeySeq::FcitxKeySeq() {}

// convert keyseq to state and sym
FcitxKeySeq::FcitxKeySeq(const char *keyseq) {
    KeyStates states;
    const char *p = keyseq;
    const char *lastModifier = keyseq;
    const char *found = nullptr;
// use macro to check modifiers
#define _CHECK_MODIFIER(NAME, VALUE)                                           \
    if ((found = strstr(p, NAME))) {                                           \
        states |= fcitx::KeyState::VALUE;                                      \
        if (found + strlen(NAME) > lastModifier) {                             \
            lastModifier = found + strlen(NAME);                               \
        }                                                                      \
    }

    _CHECK_MODIFIER("CTRL_", Ctrl)
    _CHECK_MODIFIER("Control+", Ctrl)
    _CHECK_MODIFIER("ALT_", Alt)
    _CHECK_MODIFIER("Alt+", Alt)
    _CHECK_MODIFIER("SHIFT_", Shift)
    _CHECK_MODIFIER("Shift+", Shift)
    _CHECK_MODIFIER("SUPER_", Super)
    _CHECK_MODIFIER("Super+", Super)

#undef _CHECK_MODIFIER
    sym_ = keySymFromString(lastModifier);
    states_ = states;
}

KeySym FcitxKeySeq::keySymFromString(const char *keyString) {
    auto value = std::lower_bound(
        keyValueByNameOffset,
        keyValueByNameOffset + FCITX_RIME_ARRAY_SIZE(keyValueByNameOffset),
        keyString, [](const uint32_t &idx, const std::string &str) {
            return keyNameList[&idx - keyValueByNameOffset] < str;
        });
    if (value != keyValueByNameOffset +
                     FCITX_RIME_ARRAY_SIZE(keyValueByNameOffset) &&
        strcmp(keyString, keyNameList[value - keyValueByNameOffset]) == 0) {
        return static_cast<KeySym>(*value);
    }

    return FcitxKey_None;
}

std::string FcitxKeySeq::keySymToString(KeySym sym) const {
    const KeyNameOffsetByValue *result = std::lower_bound(
        keyNameOffsetByValue,
        keyNameOffsetByValue + FCITX_RIME_ARRAY_SIZE(keyNameOffsetByValue), sym,
        [](const KeyNameOffsetByValue &item, KeySym key) {
            return item.sym < key;
        });
    if (result != keyNameOffsetByValue +
                      FCITX_RIME_ARRAY_SIZE(keyNameOffsetByValue) &&
        result->sym == sym) {
        return keyNameList[result->offset];
    }
    return std::string();
}

// convert QKeySequence to state and sym
FcitxKeySeq::FcitxKeySeq(const QKeySequence qkey) {
    int sym = 0;
    uint states = 0;
    int qkeycode = static_cast<int>(qkey[0]);
    FcitxQtKeySequenceWidget::keyQtToFcitx(
        qkeycode, FcitxQtModifierSide::MS_Unknown, sym, states);
    sym_ = static_cast<FcitxKeySym>(sym);
    states_ = static_cast<fcitx::KeyState>(states);
}

// convert to Rime X11 style string
std::string FcitxKeySeq::toString() const {
    auto sym = sym_;
    if (sym == FcitxKey_None) {
        return std::string();
    }
    if (sym == FcitxKey_ISO_Left_Tab) {
        sym = FcitxKey_Tab;
    }

    auto key = keySymToString(sym);

    if (key.empty()) {
        return std::string();
    }

    std::string str;

#define _APPEND_MODIFIER_STRING(STR, VALUE)                                    \
    if (states_ & fcitx::KeyState::VALUE) {                                    \
        str += STR;                                                            \
    }

    _APPEND_MODIFIER_STRING("Control+", Ctrl)
    _APPEND_MODIFIER_STRING("Alt+", Alt)
    _APPEND_MODIFIER_STRING("Shift+", Shift)
    _APPEND_MODIFIER_STRING("Super+", Super)

#undef _APPEND_MODIFIER_STRING
    str += key;
    return str;
}
} // namespace fcitx_rime
