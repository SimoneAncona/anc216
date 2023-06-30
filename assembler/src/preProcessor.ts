import { AssemblyObject } from "./parser";
import { generateRules, preProcessorRules } from "./rules";

export function preProcess(objs: AssemblyObject[]) {
    let preProcessorRules = generatePreProcessorRules(objs);
}

function generatePreProcessorRules(objs: AssemblyObject[]) {
    return generateRules(objs, preProcessorRules);
}