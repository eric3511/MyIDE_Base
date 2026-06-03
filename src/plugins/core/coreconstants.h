// Copyright (C) 2026 MyIDE
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
//
// Extracted from Qt Creator (coreplugin/coreconstants.h) and adapted for MyIDE.
// "QtCreator.*" identifiers are renamed to "MyIDE.*" while preserving the
// structure so that the actionmanager / idocument / ieditor wiring keeps working.

#pragma once

#include <QtGlobal>

namespace Core {
namespace Constants {

// Modes
const char MODE_WELCOME[]          = "MyIDE.WelcomeMode";
const char MODE_EDIT[]             = "MyIDE.EditMode";
const char MODE_DESIGN[]           = "MyIDE.DesignMode";
const int  P_MODE_WELCOME          = 100;
const int  P_MODE_EDIT             = 90;
const int  P_MODE_DESIGN           = 89;

// TouchBar
const char TOUCH_BAR[]             = "MyIDE.TouchBar";

// Menubar
const char MENU_BAR[]              = "MyIDE.MenuBar";

// Menus
const char M_FILE[]                = "MyIDE.Menu.File";
const char M_FILE_RECENTFILES[]    = "MyIDE.Menu.File.RecentFiles";
const char M_EDIT[]                = "MyIDE.Menu.Edit";
const char M_EDIT_ADVANCED[]       = "MyIDE.Menu.Edit.Advanced";
const char M_VIEW[]                = "MyIDE.Menu.View";
const char M_VIEW_MODESTYLES[]     = "MyIDE.Menu.View.ModeStyles";
const char M_VIEW_VIEWS[]          = "MyIDE.Menu.View.Views";
const char M_VIEW_PANES[]          = "MyIDE.Menu.View.Panes";
const char M_TOOLS[]               = "MyIDE.Menu.Tools";
const char M_TOOLS_EXTERNAL[]      = "MyIDE.Menu.Tools.External";
const char M_TOOLS_DEBUG[]         = "MyIDE.Menu.Tools.Debug";
const char M_WINDOW[]              = "MyIDE.Menu.Window";
const char M_HELP[]                = "MyIDE.Menu.Help";

// Contexts
const char C_GLOBAL[]              = "Global Context";
const char C_WELCOME_MODE[]        = "Core.WelcomeMode";
const char C_EDIT_MODE[]           = "Core.EditMode";
const char C_DESIGN_MODE[]         = "Core.DesignMode";
const char C_EDITORMANAGER[]       = "Core.EditorManager";
const char C_NAVIGATION_PANE[]     = "Core.NavigationPane";
const char C_PROBLEM_PANE[]        = "Core.ProblemPane";
const char C_GENERAL_OUTPUT_PANE[] = "Core.GeneralOutputPane";

// Default editor kind
const char K_DEFAULT_TEXT_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Plain Text Editor");
const char K_DEFAULT_TEXT_EDITOR_ID[] = "Core.PlainTextEditor";
const char K_DEFAULT_BINARY_EDITOR_ID[] = "Core.BinaryEditor";

//actions
const char UNDO[]                  = "MyIDE.Undo";
const char REDO[]                  = "MyIDE.Redo";
const char COPY[]                  = "MyIDE.Copy";
const char PASTE[]                 = "MyIDE.Paste";
const char CUT[]                   = "MyIDE.Cut";
const char SELECTALL[]             = "MyIDE.SelectAll";

const char GOTO[]                  = "MyIDE.Goto";
const char ZOOM_IN[]               = "MyIDE.ZoomIn";
const char ZOOM_OUT[]              = "MyIDE.ZoomOut";
const char ZOOM_RESET[]            = "MyIDE.ZoomReset";

const char NEW[]                   = "MyIDE.New";
const char NEW_FILE[]              = "MyIDE.NewFile";
const char OPEN[]                  = "MyIDE.Open";
const char OPEN_WITH[]             = "MyIDE.OpenWith";
const char REVERTTOSAVED[]         = "MyIDE.RevertToSaved";
const char SAVE[]                  = "MyIDE.Save";
const char SAVEAS[]                = "MyIDE.SaveAs";
const char SAVEALL[]               = "MyIDE.SaveAll";
const char PRINT[]                 = "MyIDE.Print";
const char EXIT[]                  = "MyIDE.Exit";

const char OPTIONS[]               = "MyIDE.Options";
const char LOGGER[]                = "MyIDE.Logger";
const char TOGGLE_LEFT_SIDEBAR[]   = "MyIDE.ToggleLeftSidebar";
const char TOGGLE_RIGHT_SIDEBAR[]  = "MyIDE.ToggleRightSidebar";
const char CYCLE_MODE_SELECTOR_STYLE[] =
                                     "MyIDE.CycleModeSelectorStyle";
const char TOGGLE_FULLSCREEN[]     = "MyIDE.ToggleFullScreen";
const char THEMEOPTIONS[]          = "MyIDE.ThemeOptions";

const char TR_SHOW_LEFT_SIDEBAR[]  = QT_TRANSLATE_NOOP("Core", "Show Left Sidebar");
const char TR_HIDE_LEFT_SIDEBAR[]  = QT_TRANSLATE_NOOP("Core", "Hide Left Sidebar");

const char TR_SHOW_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("Core", "Show Right Sidebar");
const char TR_HIDE_RIGHT_SIDEBAR[] = QT_TRANSLATE_NOOP("Core", "Hide Right Sidebar");

const char MINIMIZE_WINDOW[]       = "MyIDE.MinimizeWindow";
const char ZOOM_WINDOW[]           = "MyIDE.ZoomWindow";
const char CLOSE_WINDOW[]          = "MyIDE.CloseWindow";

const char SPLIT[]                 = "MyIDE.Split";
const char SPLIT_SIDE_BY_SIDE[]    = "MyIDE.SplitSideBySide";
const char SPLIT_NEW_WINDOW[]      = "MyIDE.SplitNewWindow";
const char REMOVE_CURRENT_SPLIT[]  = "MyIDE.RemoveCurrentSplit";
const char REMOVE_ALL_SPLITS[]     = "MyIDE.RemoveAllSplits";
const char GOTO_PREV_SPLIT[]       = "MyIDE.GoToPreviousSplit";
const char GOTO_NEXT_SPLIT[]       = "MyIDE.GoToNextSplit";
const char CLOSE[]                 = "MyIDE.Close";
const char CLOSE_ALTERNATIVE[]     = "MyIDE.Close_Alternative";
const char CLOSEALL[]              = "MyIDE.CloseAll";
const char CLOSEOTHERS[]           = "MyIDE.CloseOthers";
const char CLOSEALLEXCEPTVISIBLE[] = "MyIDE.CloseAllExceptVisible";
const char GOTONEXTINHISTORY[]     = "MyIDE.GotoNextInHistory";
const char GOTOPREVINHISTORY[]     = "MyIDE.GotoPreviousInHistory";
const char GO_BACK[]               = "MyIDE.GoBack";
const char GO_FORWARD[]            = "MyIDE.GoForward";
const char GOTOLASTEDIT[]          = "MyIDE.GotoLastEdit";
const char ABOUT_MYIDE[]           = "MyIDE.AboutMyIDE";
const char ABOUT_PLUGINS[]         = "MyIDE.AboutPlugins";
const char S_RETURNTOEDITOR[]      = "MyIDE.ReturnToEditor";
const char SHOWINGRAPHICALSHELL[]  = "MyIDE.ShowInGraphicalShell";
const char SHOWINFILESYSTEMVIEW[]  = "MyIDE.ShowInFileSystemView";

const char OUTPUTPANE_CLEAR[] = "Coreplugin.OutputPane.clear";

// Default groups
const char G_DEFAULT_ONE[]         = "MyIDE.Group.Default.One";
const char G_DEFAULT_TWO[]         = "MyIDE.Group.Default.Two";
const char G_DEFAULT_THREE[]       = "MyIDE.Group.Default.Three";

// Main menu bar groups
const char G_FILE[]                = "MyIDE.Group.File";
const char G_EDIT[]                = "MyIDE.Group.Edit";
const char G_VIEW[]                = "MyIDE.Group.View";
const char G_TOOLS[]               = "MyIDE.Group.Tools";
const char G_WINDOW[]              = "MyIDE.Group.Window";
const char G_HELP[]                = "MyIDE.Group.Help";

// File menu groups
const char G_FILE_NEW[]            = "MyIDE.Group.File.New";
const char G_FILE_OPEN[]           = "MyIDE.Group.File.Open";
const char G_FILE_PROJECT[]        = "MyIDE.Group.File.Project";
const char G_FILE_SAVE[]           = "MyIDE.Group.File.Save";
const char G_FILE_EXPORT[]         = "MyIDE.Group.File.Export";
const char G_FILE_CLOSE[]          = "MyIDE.Group.File.Close";
const char G_FILE_PRINT[]          = "MyIDE.Group.File.Print";
const char G_FILE_OTHER[]          = "MyIDE.Group.File.Other";

// Edit menu groups
const char G_EDIT_UNDOREDO[]       = "MyIDE.Group.Edit.UndoRedo";
const char G_EDIT_COPYPASTE[]      = "MyIDE.Group.Edit.CopyPaste";
const char G_EDIT_SELECTALL[]      = "MyIDE.Group.Edit.SelectAll";
const char G_EDIT_ADVANCED[]       = "MyIDE.Group.Edit.Advanced";

const char G_EDIT_FIND[]           = "MyIDE.Group.Edit.Find";
const char G_EDIT_OTHER[]          = "MyIDE.Group.Edit.Other";

// Advanced edit menu groups
const char G_EDIT_FORMAT[]         = "MyIDE.Group.Edit.Format";
const char G_EDIT_COLLAPSING[]     = "MyIDE.Group.Edit.Collapsing";
const char G_EDIT_TEXT[]           = "MyIDE.Group.Edit.Text";
const char G_EDIT_BLOCKS[]         = "MyIDE.Group.Edit.Blocks";
const char G_EDIT_FONT[]           = "MyIDE.Group.Edit.Font";
const char G_EDIT_EDITOR[]         = "MyIDE.Group.Edit.Editor";

// View menu groups
const char G_VIEW_VIEWS[]          = "MyIDE.Group.View.Views";
const char G_VIEW_PANES[]          = "MyIDE.Group.View.Panes";

// Tools menu groups
const char G_TOOLS_DEBUG[]         = "MyIDE.Group.Tools.Debug";
const char G_EDIT_PREFERENCES[]    = "MyIDE.Group.Edit.Preferences";

// Window menu groups
const char G_WINDOW_SIZE[]         = "MyIDE.Group.Window.Size";
const char G_WINDOW_SPLIT[]        = "MyIDE.Group.Window.Split";
const char G_WINDOW_NAVIGATE[]     = "MyIDE.Group.Window.Navigate";
const char G_WINDOW_LIST[]         = "MyIDE.Group.Window.List";
const char G_WINDOW_OTHER[]        = "MyIDE.Group.Window.Other";

// Help groups (global)
const char G_HELP_HELP[]           = "MyIDE.Group.Help.Help";
const char G_HELP_SUPPORT[]        = "MyIDE.Group.Help.Support";
const char G_HELP_ABOUT[]          = "MyIDE.Group.Help.About";
const char G_HELP_UPDATES[]        = "MyIDE.Group.Help.Updates";

// Touchbar groups
const char G_TOUCHBAR_HELP[]       = "MyIDE.Group.TouchBar.Help";
const char G_TOUCHBAR_EDITOR[]     = "MyIDE.Group.TouchBar.Editor";
const char G_TOUCHBAR_NAVIGATION[] = "MyIDE.Group.TouchBar.Navigation";
const char G_TOUCHBAR_OTHER[]      = "MyIDE.Group.TouchBar.Other";

const char WIZARD_CATEGORY_QT[] = "R.Qt";
const char WIZARD_TR_CATEGORY_QT[] = QT_TRANSLATE_NOOP("Core", "Qt");
const char WIZARD_KIND_UNKNOWN[] = "unknown";
const char WIZARD_KIND_PROJECT[] = "project";
const char WIZARD_KIND_FILE[] = "file";

const char SETTINGS_CATEGORY_CORE[] = "B.Core";
const char SETTINGS_ID_INTERFACE[] = "A.Interface";
const char SETTINGS_ID_SYSTEM[] = "B.Core.System";
const char SETTINGS_ID_SHORTCUTS[] = "C.Keyboard";
const char SETTINGS_ID_TOOLS[] = "D.ExternalTools";
const char SETTINGS_ID_MIMETYPES[] = "E.MimeTypes";

const char SETTINGS_DEFAULTTEXTENCODING[] = "General/DefaultFileEncoding";
const char SETTINGS_DEFAULT_LINE_TERMINATOR[] = "General/DefaultLineTerminator";

const char SETTINGS_THEME[] = "Core/CreatorTheme";
const char DEFAULT_THEME[] = "flat";
const char DEFAULT_DARK_THEME[] = "flat-dark";

const char TR_CLEAR_MENU[]         = QT_TRANSLATE_NOOP("Core", "Clear Menu");

const int MODEBAR_ICON_SIZE = 34;
const int MODEBAR_ICONSONLY_BUTTON_SIZE = MODEBAR_ICON_SIZE + 4;
const int DEFAULT_MAX_CHAR_COUNT = 10000000;

} // namespace Constants
} // namespace Core
