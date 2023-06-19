export type ModuleInfo = {
    name: string;
    path: string;
    isMain: boolean;
}

let modules: ModuleInfo[] = [];
export let currentModule: ModuleInfo;

export function registerModule(module: ModuleInfo): void {
    modules.push(module);
}

export function getMainModule(): ModuleInfo | null {
    for (let module of modules) if (module.isMain) return module;
    return null;
}

export function setCurrentModule(module: ModuleInfo) {
    currentModule = module;
}