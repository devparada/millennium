import ProtocolMapping from 'devtools-protocol/types/protocol-mapping';

/** Returnable IPC types */
type IPCType = string | number | boolean | void;

/*
 Global Millennium API for developers.
*/
type Millennium = {
	/**
	 * Call a method on the backend
	 * @deprecated Use `callable` instead.
	 * Example usage:
	 * ```typescript
	 * // before
	 * const result = await Millennium.callServerMethod('methodName', { arg1: 'value' });
	 * // after
	 * const method = callable<[{ arg1: string }]>("methodName");
	 *
	 * const result1 = await method({ arg1: 'value1' });
	 * const result2 = await method({ arg1: 'value2' });
	 * ```
	 */
	callServerMethod: (methodName: string, kwargs?: object) => Promise<any>;
	findElement: (privateDocument: Document, querySelector: string, timeOut?: number) => Promise<NodeListOf<Element>>;
	exposeObj?: <T extends object>(obj: T) => any;
	AddWindowCreateHook?: (callback: (context: object) => void) => void;
};

/**
 * Make reusable IPC call declarations
 *
 * frontend:
 * ```typescript
 * const method = callable<[{ arg1: string }]>("methodName"); // declare the method
 * method({ arg1: 'value' }); // call the method
 * ```
 *
 * backend:
 * ```python
 * def methodName(arg1: str):
 *    pass
 * ```
 */
const callable: <
	// Ideally this would be `Params extends Record<...>` but for backwards compatibility we keep a tuple type
	Params extends [params: Record<string, IPCType>] | [] = [],
	Return extends IPCType = IPCType,
>(
	route: string,
) => (...params: Params) => Promise<Return> =
	(_route: string) =>
	(..._params: any[]) =>
		Promise.resolve(undefined as any);

const m_private_context: any = undefined;
export const pluginSelf = m_private_context;

declare global {
	interface Window {
		Millennium: Millennium;
		MILLENNIUM_CHROME_DEV_TOOLS_PROTOCOL_DO_NOT_USE_OR_OVERRIDE_ONMESSAGE: any;
		CHROME_DEV_TOOLS_PROTOCOL_API_BINDING: MillenniumChromeDevToolsProtocol;
	}
}

const BindPluginSettings: () => any = (): any => undefined;

const pluginConfig: {
	get: <T = any>(key: string) => Promise<T>;
	set: (key: string, value: any) => Promise<void>;
	delete: (key: string) => Promise<void>;
	getAll: <T = Record<string, any>>() => Promise<T>;
} = { get: async () => undefined as any, set: async () => {}, delete: async () => {}, getAll: async () => ({}) as any };

const usePluginConfig: {
	<T = any>(key: string): [T | undefined, (value: T) => Promise<void>];
	(): [Record<string, any>, (key: string, value: any) => Promise<void>];
} = (() => [undefined, async () => {}]) as any;

const subscribePluginConfig: (cb: (key: string, value: any) => void) => () => void = () => () => {};

class MillenniumChromeDevToolsProtocol {
	currentId: number;
	pendingRequests: Map<number, { resolve: (value: any) => void; reject: (reason?: any) => void }>;
	eventListeners: Map<string, Set<(params: any) => void>>;

	constructor() {
		this.currentId = 0;
		this.pendingRequests = new Map();
		this.eventListeners = new Map();

		window.MILLENNIUM_CHROME_DEV_TOOLS_PROTOCOL_DO_NOT_USE_OR_OVERRIDE_ONMESSAGE = {
			__handleCDPResponse: (response: any) => {
				this.handleResponse(response);
			},
		};
	}

	handleResponse(response: any) {
		const data = typeof response === 'string' ? JSON.parse(response) : response;

		if (data.call_id !== undefined && this.pendingRequests.has(data.call_id)) {
			const pending = this.pendingRequests.get(data.call_id)!;
			this.pendingRequests.delete(data.call_id);

			if (data.error) {
				pending.reject(new Error(`CDP Error: ${data.error.message}`));
			} else {
				pending.resolve(data.result);
			}
		}
	}

	on<T extends keyof ProtocolMapping.Events>(event: T, listener: (params: ProtocolMapping.Events[T][0]) => void): () => void {
		if (!this.eventListeners.has(event)) {
			this.eventListeners.set(event, new Set());
		}
		this.eventListeners.get(event)!.add(listener);
		return () => this.off(event, listener);
	}

	off<T extends keyof ProtocolMapping.Events>(event: T, listener: (params: ProtocolMapping.Events[T][0]) => void) {
		const listeners = this.eventListeners.get(event);
		if (listeners) {
			listeners.delete(listener);
			if (listeners.size === 0) {
				this.eventListeners.delete(event);
			}
		}
	}

	send<T extends keyof ProtocolMapping.Commands>(
		method: T,
		params: ProtocolMapping.Commands[T]['paramsType'][0] = {},
		sessionId?: string,
	): Promise<ProtocolMapping.Commands[T]['returnType']> {
		return new Promise((resolve, reject) => {
			const call_id = this.currentId++;
			this.pendingRequests.set(call_id, { resolve, reject });

			const payload: any = { call_id, method };
			if (params && Object.keys(params).length > 0) {
				payload.params = params;
			}
			if (sessionId) {
				payload.sessionId = sessionId;
			}

			try {
				(window as any).__millennium_cdp_send__(JSON.stringify(payload));
			} catch (error) {
				this.pendingRequests.delete(call_id);
				reject(error);
			}
		});
	}

	sendNoResponse<T extends keyof ProtocolMapping.Commands>(method: T, params: ProtocolMapping.Commands[T]['paramsType'][0] = {}) {
		const payload: any = { call_id: this.currentId++, method };
		if (params && Object.keys(params).length > 0) {
			payload.params = params;
		}
		(window as any).__millennium_cdp_send__(JSON.stringify(payload));
	}
}

const ChromeDevToolsProtocol: MillenniumChromeDevToolsProtocol = new MillenniumChromeDevToolsProtocol();
const Millennium: Millennium = window.Millennium;
export { BindPluginSettings, callable, ChromeDevToolsProtocol, Millennium, pluginConfig, subscribePluginConfig, usePluginConfig };
