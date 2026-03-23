import { FC, SVGAttributes } from 'react';
import { findModuleDetailsByExport } from '../webpack';

const __iconsModule = findModuleDetailsByExport(
	(e) => e?.toString && (/Spinner\)}\)?,.\.createElement\(\"path\",{d:\"M18 /.test(e.toString()) || /Spinner\).*\.jsxs?\)\("path",\{d:"M18 /s.test(e.toString())),
);

export const IconsModule = __iconsModule[0];
export const Spinner = __iconsModule[1] as FC<SVGAttributes<SVGElement>>;
