type SymbolInfo = {
    module: string;
    name: string;
    isLocal: boolean;
    type: "string"
    value: string;
} | {
    module: string;
    name: string;
    isLocal: boolean;
    type: "number"
    value: number;
}

let symbolTable: SymbolInfo[] = []