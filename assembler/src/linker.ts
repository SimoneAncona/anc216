export type ModuleInfo = {
    name: string;
    path: string;
}

let modules: ModuleInfo[] = [];

export function registerModule(module: ModuleInfo): void {
    modules.push(module);
}