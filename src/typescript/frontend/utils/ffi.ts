/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2026 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

import { callable } from '@steambrew/client';

export const Core_GetSteamPath = callable<[], string>('Core_GetSteamPath');
export const Core_FindAllPlugins = callable<[], string>('Core_FindAllPlugins');
export const Core_FindAllThemes = callable<[], string>('Core_FindAllThemes');
export const Core_ChangePluginStatus = callable<[{ pluginJson: string }], any>('Core_ChangePluginStatus');
export const Core_GetEnvironmentVar = callable<[{ variable: string }], string>('Core_GetEnvironmentVar');
export const Core_InstallTheme = callable<[{ owner: string; repo: string }], void>('Core_InstallTheme');
export const Core_UninstallTheme = callable<[{ owner: string; repo: string }], void>('Core_UninstallTheme');
export const Core_IsThemeInstalled = callable<[{ owner: string; repo: string }], boolean>('Core_IsThemeInstalled');
export const Core_GetThemeFromGitPair = callable<[{ repo: string; owner: string; asString: boolean }], string>('Core_GetThemeFromGitPair');
export const Core_UninstallPlugin = callable<[{ pluginName: string }], string>('Core_UninstallPlugin');
export const Core_DownloadThemeUpdate = callable<[{ native: string }], boolean>('Core_DownloadThemeUpdate');
export const Core_DownloadPluginUpdate = callable<[{ id: string; name: string; commit: string }], boolean>('Core_DownloadPluginUpdate');
export const Core_KillPluginBackend = callable<[{ pluginName: string }], void>('Core_KillPluginBackend');
export const Core_GetUpdates = callable<[{ force: boolean }], string>('Core_GetUpdates');
export const Core_InstallPlugin = callable<[{ download_url: string; total_size: number }], void>('Core_InstallPlugin');
export const Core_IsPluginInstalled = callable<[{ plugin_name: string }], boolean>('Core_IsPluginInstalled');
export const Core_GetActiveTheme = callable<[], string>('Core_GetActiveTheme');
export const Core_GetThemeColorOptions = callable<[{ theme_name: string }], string>('Core_GetThemeColorOptions');
export const Core_DoesThemeUseAccentColor = callable<[]>('Core_DoesThemeUseAccentColor');
export const Core_SetBackendConfig = callable<[{ config: string; skip_propagation: true }], void>('Core_SetBackendConfig');
export const Core_GetBackendConfig = callable<[], string>('Core_GetBackendConfig');
export const Core_GetStartConfig = callable<[], string>('Core_GetStartConfig');
export const Core_GetPluginBackendLogs = callable<[], any>('Core_GetPluginBackendLogs');
export const Core_GetRootColors = callable<[], string>('Core_GetRootColors');
export const Core_ChangeCondition = callable<[{ theme: string; newData: string; condition: string }], boolean>('Core_ChangeCondition');
export const Core_ChangeColor = callable<[{ theme: string; new_color: string; color_name: string; color_type: number }], string>('Core_ChangeColor');
export const Core_ChangeAccentColor = callable<[{ new_color: string }], string>('Core_ChangeAccentColor');
export const Core_UpdateMillennium = callable<[{ downloadUrl: string; downloadSize: number; background: boolean }], void>('Core_UpdateMillennium');
export const Core_HasPendingMillenniumUpdateRestart = callable<[], boolean>('Core_HasPendingMillenniumUpdateRestart');
export const Core_GetPendingCrashes = callable<[], string>('Core_GetPendingCrashes');
export const Core_AcknowledgeCrash = callable<[{ plugin: string }], void>('Core_AcknowledgeCrash');
export const PluginConfig_GetAll = callable<[{ pluginName: string }], string>('PluginConfig_GetAll');
export const PluginConfig_DeleteAll = callable<[{ pluginName: string }], string>('PluginConfig_DeleteAll');
