import del from 'rollup-plugin-del';
import terser from '@rollup/plugin-terser';
import typescript from '@rollup/plugin-typescript';

export default {
	input: 'src/index.ts',
	context: 'window',
	output: {
		file: 'build/cdp-isolated-ctx.js',
		format: 'iife',
		sourcemap: true,
	},
	onwarn(warning, warn) {
		throw new Error(warning.message);
	},
	plugins: [
		del({ targets: ['build/*', 'build/.*'], runOnce: true }),
		typescript(),
		terser(),
	],
};
