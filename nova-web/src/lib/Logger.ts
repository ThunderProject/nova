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
        const decoratedMsg = `${msg}`

        NovaApi.Log(level.toString(), decoratedMsg);

        switch (level) {
            case loglevel.debug:
                console.debug(decoratedMsg);
                break
            case loglevel.info:
                console.info(decoratedMsg);
                break;
            case loglevel.warn:
                console.warn(decoratedMsg);
                break;
            case loglevel.error:
            case loglevel.fatal:
                console.error(decoratedMsg)
                break;
        }
    };

    static debug = <Type>(msg: Type): void => logger.#get().#log(loglevel.debug,msg);
    static info = <Type>(msg: Type): void => logger.#get().#log(loglevel.info,msg);
    static warn = <Type>(msg: Type): void => logger.#get().#log(loglevel.warn,msg);
    static error = <Type>(msg: Type): void => logger.#get().#log(loglevel.error,msg);
    static fatal = <Type>(msg: Type): void => logger.#get().#log(loglevel.fatal,msg);
}