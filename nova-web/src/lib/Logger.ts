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

    static #colors = {
        [loglevel.debug]: '\x1b[32m',
        [loglevel.info]: '\x1b[34m',
        [loglevel.warn]: '\x1b[33m',
        [loglevel.error]: '\x1b[31m',
        [loglevel.fatal]: '\x1b[31m',
    };

    static #reset = '\x1b[0m';

    #log = <Type>(level: loglevel, msg: Type): void => {
        const color = logger.#colors[level] ||  logger.#reset;
        const decoratedMsg = `${color}<${level}> ${new Date().toISOString()}: ${msg}${logger.#reset}`

        switch (level) {
            case loglevel.debug: return console.debug(decoratedMsg)
            case loglevel.info: return console.info(decoratedMsg)
            case loglevel.warn: return console.warn(decoratedMsg)
            case loglevel.error: return console.error(decoratedMsg)
            case loglevel.fatal: return console.error(decoratedMsg)
        }
    };

    static debug = <Type>(msg: Type): void => logger.#get().#log(loglevel.debug,msg);
    static info = <Type>(msg: Type): void => logger.#get().#log(loglevel.info,msg);
    static warn = <Type>(msg: Type): void => logger.#get().#log(loglevel.warn,msg);
    static error = <Type>(msg: Type): void => logger.#get().#log(loglevel.error,msg);
    static fatal = <Type>(msg: Type): void => logger.#get().#log(loglevel.fatal,msg);
}