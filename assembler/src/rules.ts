type RuleName =
    ""

type RuleAttributeType = 
    "identifier" 
    | "literalNumber" 
    | "literalString" 
    | "keyword"

export class Rule {
    private name: RuleName;
    private attributes: RuleAttribute[];

    constructor(name: RuleName, attributes: RuleAttribute[]) {
        this.name = name;
        this.attributes = attributes;
    }

    getName(): RuleName { return this.name; }

    getAttributes(): RuleAttribute[] {
        return this.attributes;
    }

    getAttribute(name: string): RuleAttribute | null {
        for (const attribute of this.attributes) {
            if (attribute.name === name) return attribute;
        };
        return null;
    }
}

export type RuleAttribute = {
    name: string;
    value: string;
    type: RuleAttributeType;
}