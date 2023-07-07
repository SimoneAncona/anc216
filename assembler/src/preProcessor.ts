import * as ohm from 'ohm-js';

export function preProcess(matches: ohm.MatchResult[], semantics: ohm.Semantics) {
    semantics.addOperation("preProcess", {
        _iter(...children) {
            children.forEach(c => c.preProcess());
        },
        Line(statements) {
            return statements.asIteration().children.forEach(c => c.preProcess());
        },
        Statement(statement) {
            return statement.preProcess();
        },
        Label(name, _end) {
            console.log(name);
        },
        Section(_section, sectionName, _end) {
            console.log("section", sectionName);
            return sectionName;
        },
        Exp(exp) {

        },
        UseAs(_use, id, _as, value, _end) {
            console.log("use", id, "as", value);
            return id;
        }
    });

    for (let match of matches) {
        console.log(semantics(match).preProcess());
    }
}