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

import { ffi } from '@steambrew/client';

export const Core_GetSteamPath = ffi<[], string>('Core_GetSteamPath');
export const Core_FindAllPlugins = ffi<[], any>('Core_FindAllPlugins');
export const Core_FindAllThemes = ffi<[], any>('Core_FindAllThemes');
export const Core_ChangePluginStatus = ffi<[pluginJson: string], any>('Core_ChangePluginStatus');
export const Core_GetEnvironmentVar = ffi<[variable: string], string>('Core_GetEnvironmentVar');
export const Core_InstallTheme = ffi<[owner: string, repo: string], void>('Core_InstallTheme');
export const Core_UninstallTheme = ffi<[owner: string, repo: string], void>('Core_UninstallTheme');
export const Core_IsThemeInstalled = ffi<[owner: string, repo: string], any>('Core_IsThemeInstalled');
export const Core_GetThemeFromGitPair = ffi<[repo: string, owner: string, asString: boolean], any>('Core_GetThemeFromGitPair');
export const Core_UninstallPlugin = ffi<[pluginName: string], any>('Core_UninstallPlugin');
export const Core_DownloadThemeUpdate = ffi<[native: string], boolean>('Core_DownloadThemeUpdate');
export const Core_DownloadPluginUpdate = ffi<[id: string, name: string, commit: string], boolean>('Core_DownloadPluginUpdate');
export const Core_KillPluginBackend = ffi<[pluginName: string], void>('Core_KillPluginBackend');
export const Core_GetUpdates = ffi<[force: boolean], any>('Core_GetUpdates');
export const Core_InstallPlugin = ffi<[downloadUrl: string, totalSize: number], void>('Core_InstallPlugin');
export const Core_IsPluginInstalled = ffi<[pluginName: string], any>('Core_IsPluginInstalled');
export const Core_GetActiveTheme = ffi<[], string>('Core_GetActiveTheme');
export const Core_GetThemeColorOptions = ffi<[themeName: string], any>('Core_GetThemeColorOptions');
export const Core_DoesThemeUseAccentColor = ffi<[]>('Core_DoesThemeUseAccentColor');
export const Core_SetBackendConfig = ffi<[config: string, skipPropagation: boolean], void>('Core_SetBackendConfig');
export const Core_GetBackendConfig = ffi<[], any>('Core_GetBackendConfig');
export const Core_GetStartConfig = ffi<[], any>('Core_GetStartConfig');
export const Core_GetPluginBackendLogs = ffi<[], any>('Core_GetPluginBackendLogs');
export const Core_GetRootColors = ffi<[], any>('Core_GetRootColors');
export const Core_ChangeCondition = ffi<[theme: string, newData: string, condition: string], boolean>('Core_ChangeCondition');
export const Core_ChangeColor = ffi<[theme: string, colorName: string, newColor: string, colorType: number], any>('Core_ChangeColor');
export const Core_ChangeAccentColor = ffi<[newColor: string], any>('Core_ChangeAccentColor');
export const Core_UpdateMillennium = ffi<[downloadUrl: string, downloadSize: number, background: boolean], void>('Core_UpdateMillennium');
export const Core_HasPendingMillenniumUpdateRestart = ffi<[], boolean>('Core_HasPendingMillenniumUpdateRestart');
export const Core_GetPendingCrashes = ffi<[], any>('Core_GetPendingCrashes');
export const Core_AcknowledgeCrash = ffi<[plugin: string], void>('Core_AcknowledgeCrash');
export const Core_WatchQuickCss = ffi<[]>('Core_WatchQuickCss');
export const Core_UnwatchQuickCss = ffi<[]>('Core_UnwatchQuickCss');
export const PluginConfig_GetAll = ffi<[pluginName: string], any>('PluginConfig_GetAll');
export const PluginConfig_DeleteAll = ffi<[pluginName: string], any>('PluginConfig_DeleteAll');
