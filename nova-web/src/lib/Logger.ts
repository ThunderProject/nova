import {NovaApi} from "../nova_api/NovaApi.ts";

enum loglevel {
    debug = "debug",
    info = "info",
    warn = "warn",
    error = "error",
    fatal = "fatal",
}

export class logger {
    static #instance: logger | null = null;
    private constructor() {}

    static #get = (): logger => {
        if(!logger.#instance) {
            logger.#instance = new logger();
        }
        return logger.#instance;
    }

    #log = <Type>(level: loglevel, msg: Type): void => {
        NovaApi.Log(level.toString(), `${msg}`);
    };

    static debug = <Type>(msg: Type): void => logger.#get().#log(loglevel.debug,msg);
    static info = <Type>(msg: Type): void => logger.#get().#log(loglevel.info,msg);
    static warn = <Type>(msg: Type): void => logger.#get().#log(loglevel.warn,msg);
    static error = <Type>(msg: Type): void => logger.#get().#log(loglevel.error,msg);
    static fatal = <Type>(msg: Type): void => logger.#get().#log(loglevel.fatal,msg);
}