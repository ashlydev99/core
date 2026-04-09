import { StdioNexusChat } from "@nexuschat/jsonrpc-client";

export interface SearchOptions {
  /** whether take nexuschat-rpc-server inside of $PATH*/
  takeVersionFromPATH: boolean;

  /** whether to disable the NEXUS_CHAT_RPC_SERVER environment variable */
  disableEnvPath: boolean;
}

/**
 * 
 * @returns absolute path to deltachat-rpc-server binary
 * @throws when it is not found
 */
export function getRPCServerPath(
  options?: Partial<SearchOptions>
): Promise<string>;



export type NexusChatOverJsonRpcServer = StdioNexusChat & {
    readonly pathToServerBinary: string;
};

export interface StartOptions {
  /** whether to disable outputting stderr to the parent process's stderr */
  muteStdErr: boolean;
}

/**
 * 
 * @param directory directory for accounts folder
 * @param options 
 */
export function startNexusChat(directory: string, options?: Partial<SearchOptions & StartOptions> ): Promise<NexusChatOverJsonRpcServer>


export namespace FnTypes {
    export type getRPCServerPath = typeof getRPCServerPath
    export type startNexusChat = typeof startNexusChat
}