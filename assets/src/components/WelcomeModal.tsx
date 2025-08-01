/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2025 Project Millennium
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

import { ConfirmModal, pluginSelf, showModal, ShowModalResult } from '@steambrew/client';
import { settingsManager } from '../settings-manager';
import Markdown from 'markdown-to-jsx';
import { locale } from '../../locales';
import { useEffect } from 'react';

export const WelcomeModalComponent = (): JSX.Element => {
	useEffect(() => {
		if (settingsManager.config.misc.hasShownWelcomeModal) {
			return;
		} else {
			settingsManager.config.misc.hasShownWelcomeModal = true;
			settingsManager.forceSaveConfig();
		}

		let welcomeModalWindow: ShowModalResult;

		const WelcomeModal = () => {
			return (
				<ConfirmModal
					strTitle={locale.strWelcomeModalTitle}
					strDescription={<Markdown options={{ overrides: { a: { props: { target: '_blank' } } } }}>{locale.strWelcomeModalDescription}</Markdown>}
					strOKButtonText={locale.strWelcomeModalOKButton}
					bAlertDialog={true}
					bDisableBackgroundDismiss={true}
					bHideCloseIcon={true}
					onOK={() => {
						welcomeModalWindow?.Close();
					}}
				/>
			);
		};

		welcomeModalWindow = showModal(<WelcomeModal />, pluginSelf.mainWindow, {
			popupHeight: 475,
			popupWidth: 625,
		});
	}, []);

	return null;
};
