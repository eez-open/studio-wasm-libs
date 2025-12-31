/**
 * Tests for script-compiler.js
 * Run with: node test-script-compiler.js
 */

// Load the compiler
const { compile_to_javascript, Lexer, Parser, Interpreter } = require('./script-compiler.js');

// Test framework
let testCount = 0;
let passCount = 0;
let failCount = 0;

function test(name, fn) {
    testCount++;
    try {
        fn();
        passCount++;
        console.log(`✓ ${name}`);
    } catch (error) {
        failCount++;
        console.log(`✗ ${name}`);
        console.log(`  Error: ${error.message}`);
        if (error.stack) {
            console.log(`  ${error.stack.split('\n').slice(1, 3).join('\n  ')}`);
        }
    }
}

function assert(condition, message) {
    if (!condition) {
        throw new Error(message || 'Assertion failed');
    }
}

function assertEquals(actual, expected, message) {
    if (actual !== expected) {
        throw new Error(message || `Expected ${expected} but got ${actual}`);
    }
}

// Mock LVGL runtime and constants
const mockLvgl = {
    _lv_obj_create: (parent) => ({ type: 'obj', parent }),
    _lv_button_create: (parent) => ({ type: 'button', parent }),
    _lv_label_create: (parent) => ({ type: 'label', parent }),
    _lv_obj_set_pos: (obj, x, y) => { obj.x = x; obj.y = y; return obj; },
    _lv_obj_set_size: (obj, w, h) => { obj.width = w; obj.height = h; return obj; },
    _lv_obj_set_style_align: (obj, align, selector) => { obj.align = align; obj.selector = selector; return obj; },
    _lv_label_set_text: (label, text) => { label.text = text; return label; },
    _lv_screen_load_anim: (screen, anim, time, delay, auto_del) => { 
        screen.anim = anim; 
        screen.time = time;
        return screen; 
    },
    stringToNewUTF8: (str) => str
};

const mockConstants = {
    LV_SIZE_CONTENT: 1073741823,
    LV_ALIGN_CENTER: 9,
    LV_PART_MAIN: 0,
    LV_STATE_DEFAULT: 0,
    LV_SCR_LOAD_ANIM_FADE_IN: 9
};

console.log('\n=== Testing Script Compiler ===\n');

// ============================================================================
// LEXER TESTS
// ============================================================================

console.log('--- Lexer Tests ---');

test('Lexer: tokenize numbers', () => {
    const lexer = new Lexer('123 45.67');
    const tokens = lexer.tokenize();
    assertEquals(tokens[0].type, 'NUMBER');
    assertEquals(tokens[0].value, 123);
    assertEquals(tokens[1].type, 'NUMBER');
    assertEquals(tokens[1].value, 45.67);
});

test('Lexer: tokenize strings', () => {
    const lexer = new Lexer('"hello" \'world\'');
    const tokens = lexer.tokenize();
    assertEquals(tokens[0].type, 'STRING');
    assertEquals(tokens[0].value, 'hello');
    assertEquals(tokens[1].type, 'STRING');
    assertEquals(tokens[1].value, 'world');
});

test('Lexer: tokenize identifiers and keywords', () => {
    const lexer = new Lexer('function myFunc let const if');
    const tokens = lexer.tokenize();
    assertEquals(tokens[0].type, 'FUNCTION');
    assertEquals(tokens[1].type, 'IDENTIFIER');
    assertEquals(tokens[1].value, 'myFunc');
    assertEquals(tokens[2].type, 'LET');
    assertEquals(tokens[3].type, 'CONST');
    assertEquals(tokens[4].type, 'IF');
});

test('Lexer: tokenize operators', () => {
    const lexer = new Lexer('+ - * / % == != <= >= && || | & ^');
    const tokens = lexer.tokenize();
    assertEquals(tokens[0].type, 'PLUS');
    assertEquals(tokens[1].type, 'MINUS');
    assertEquals(tokens[2].type, 'STAR');
    assertEquals(tokens[3].type, 'SLASH');
    assertEquals(tokens[4].type, 'PERCENT');
    assertEquals(tokens[5].type, '==');
    assertEquals(tokens[6].type, '!=');
    assertEquals(tokens[7].type, '<=');
    assertEquals(tokens[8].type, '>=');
    assertEquals(tokens[9].type, '&&');
    assertEquals(tokens[10].type, '||');
    assertEquals(tokens[11].type, 'PIPE');
    assertEquals(tokens[12].type, 'AMP');
    assertEquals(tokens[13].type, 'CARET');
});

test('Lexer: skip comments', () => {
    const lexer = new Lexer('1 // comment\n2 /* block */ 3');
    const tokens = lexer.tokenize();
    assertEquals(tokens[0].value, 1);
    assertEquals(tokens[1].value, 2);
    assertEquals(tokens[2].value, 3);
});

// ============================================================================
// PARSER TESTS
// ============================================================================

console.log('\n--- Parser Tests ---');

test('Parser: parse function declaration', () => {
    const code = 'function test(a, b) { return a + b; }';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body.length, 1);
    assertEquals(ast.body[0].type, 'FunctionDeclaration');
    assertEquals(ast.body[0].name, 'test');
    assertEquals(ast.body[0].params.length, 2);
});

test('Parser: parse variable declarations', () => {
    const code = 'let x = 5; const y = 10;';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body.length, 2);
    assertEquals(ast.body[0].type, 'VariableDeclaration');
    assertEquals(ast.body[0].kind, 'let');
    assertEquals(ast.body[0].name, 'x');
    assertEquals(ast.body[1].kind, 'const');
});

test('Parser: parse if statement', () => {
    const code = 'if (x > 5) { return true; } else { return false; }';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].type, 'IfStatement');
    assert(ast.body[0].test !== null);
    assert(ast.body[0].consequent !== null);
    assert(ast.body[0].alternate !== null);
});

test('Parser: parse for loop', () => {
    const code = 'for (let i = 0; i < 10; i++) { sum = sum + i; }';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].type, 'ForStatement');
    assert(ast.body[0].init !== null);
    assert(ast.body[0].test !== null);
    assert(ast.body[0].update !== null);
});

test('Parser: parse while loop', () => {
    const code = 'while (x < 10) { x++; }';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].type, 'WhileStatement');
    assert(ast.body[0].test !== null);
    assert(ast.body[0].body !== null);
});

test('Parser: parse bitwise OR expression', () => {
    const code = 'let x = LV_PART_MAIN | LV_STATE_DEFAULT;';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].type, 'VariableDeclaration');
    assertEquals(ast.body[0].init.type, 'BinaryExpression');
    assertEquals(ast.body[0].init.operator, '|');
});

test('Parser: parse function call', () => {
    const code = 'lv_obj_create(parent);';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].type, 'ExpressionStatement');
    assertEquals(ast.body[0].expression.type, 'CallExpression');
    assertEquals(ast.body[0].expression.callee.name, 'lv_obj_create');
});

test('Parser: parse member expression', () => {
    const code = 'obj.property';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].expression.type, 'MemberExpression');
    assertEquals(ast.body[0].expression.object.name, 'obj');
    assertEquals(ast.body[0].expression.property, 'property');
});

// ============================================================================
// INTERPRETER TESTS
// ============================================================================

console.log('\n--- Interpreter Tests ---');

test('Interpreter: execute simple arithmetic', () => {
    const script = compile_to_javascript(`
        function add(a, b) {
            return a + b;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('add', 5, 3);
    assertEquals(result, 8);
});

test('Interpreter: execute with variables', () => {
    const script = compile_to_javascript(`
        function test() {
            let x = 10;
            let y = 20;
            return x + y;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 30);
});

test('Interpreter: execute if statement', () => {
    const script = compile_to_javascript(`
        function max(a, b) {
            if (a > b) {
                return a;
            } else {
                return b;
            }
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    assertEquals(script.exec('max', 10, 5), 10);
    assertEquals(script.exec('max', 3, 8), 8);
});

test('Interpreter: execute for loop', () => {
    const script = compile_to_javascript(`
        function sum(n) {
            let result = 0;
            for (let i = 0; i <= n; i++) {
                result = result + i;
            }
            return result;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('sum', 10);
    assertEquals(result, 55); // 1+2+3+...+10 = 55
});

test('Interpreter: execute while loop', () => {
    const script = compile_to_javascript(`
        function countdown(n) {
            let count = n;
            while (count > 0) {
                count--;
            }
            return count;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('countdown', 5);
    assertEquals(result, 0);
});

test('Interpreter: bitwise OR operator', () => {
    const script = compile_to_javascript(`
        function combine() {
            return LV_PART_MAIN | LV_STATE_DEFAULT;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('combine');
    assertEquals(result, 0 | 0); // Both are 0, so result is 0
});

test('Interpreter: bitwise AND operator', () => {
    const script = compile_to_javascript(`
        function test() {
            return 5 & 3;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 1); // 5 & 3 = 1
});

test('Interpreter: bitwise XOR operator', () => {
    const script = compile_to_javascript(`
        function test() {
            return 5 ^ 3;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 6); // 5 ^ 3 = 6
});

test('Interpreter: call lv_* functions', () => {
    const script = compile_to_javascript(`
        function create() {
            let obj = lv_obj_create(0);
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('create');
    assertEquals(result.type, 'obj');
    assertEquals(result.parent, 0);
});

test('Interpreter: nested function calls', () => {
    const script = compile_to_javascript(`
        function helper(x) {
            return x * 2;
        }
        
        function main(y) {
            return helper(y) + 5;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('main', 10);
    assertEquals(result, 25); // (10 * 2) + 5 = 25
});

test('Interpreter: complex LVGL example', () => {
    const script = compile_to_javascript(`
        function create_button(parent) {
            let button = lv_button_create(parent);
            lv_obj_set_pos(button, 100, 200);
            lv_obj_set_size(button, 150, 50);
            
            let label = lv_label_create(button);
            lv_obj_set_style_align(label, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(label, "Click Me");
            
            return button;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const button = script.exec('create_button', null);
    
    assertEquals(button.type, 'button');
    assertEquals(button.x, 100);
    assertEquals(button.y, 200);
    assertEquals(button.width, 150);
    assertEquals(button.height, 50);
});

test('Interpreter: member access', () => {
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 10, 20);
            return obj.x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 10);
});

test('Interpreter: string literals', () => {
    const script = compile_to_javascript(`
        function get_text() {
            return "Hello World";
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('get_text');
    assertEquals(result, "Hello World");
});

test('Interpreter: logical operators', () => {
    const script = compile_to_javascript(`
        function test_and(a, b) {
            return a && b;
        }
        function test_or(a, b) {
            return a || b;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    assertEquals(script.exec('test_and', true, true), true);
    assertEquals(script.exec('test_and', true, false), false);
    assertEquals(script.exec('test_or', false, true), true);
    assertEquals(script.exec('test_or', false, false), false);
});

test('Interpreter: comparison operators', () => {
    const script = compile_to_javascript(`
        function test(a, b) {
            return a < b;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    assertEquals(script.exec('test', 5, 10), true);
    assertEquals(script.exec('test', 10, 5), false);
});

test('Interpreter: compound assignment', () => {
    const script = compile_to_javascript(`
        function test() {
            let x = 10;
            x += 5;
            return x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 15);
});

test('Interpreter: unary operators', () => {
    const script = compile_to_javascript(`
        function test_not(x) {
            return !x;
        }
        function test_neg(x) {
            return -x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    assertEquals(script.exec('test_not', true), false);
    assertEquals(script.exec('test_not', false), true);
    assertEquals(script.exec('test_neg', 10), -10);
});

test('Interpreter: increment/decrement', () => {
    const script = compile_to_javascript(`
        function test_inc() {
            let x = 5;
            ++x;
            return x;
        }
        function test_dec() {
            let x = 5;
            x--;
            return x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    assertEquals(script.exec('test_inc'), 6);
    assertEquals(script.exec('test_dec'), 4);
});

test('Interpreter: access globals', () => {
    const globals = {
        System: {
            version: '1.0.0',
            add: (a, b) => a + b
        }
    };
    const script = compile_to_javascript(`
        function get_version() {
            return System.version;
        }
        function call_add(x, y) {
            return System.add(x, y);
        }
    `);
    script.init(globals, mockLvgl, mockConstants);
    assertEquals(script.exec('get_version'), '1.0.0');
    assertEquals(script.exec('call_add', 10, 20), 30);
});

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

console.log('\n--- Error Handling Tests ---');

test('Error: undefined variable', () => {
    const script = compile_to_javascript(`
        function test() {
            return undefinedVar;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('Undefined variable'));
    }
});

test('Error: undefined function', () => {
    const script = compile_to_javascript(`
        function test() {
            return 42;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('nonexistent');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('Function not found'));
    }
});

test('Error: unknown LVGL function', () => {
    const script = compile_to_javascript(`
        function test() {
            return lv_nonexistent_function();
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('Unknown LVGL function'));
    }
});

test('Error: LVGL function not in allowed list', () => {
    const allowedFunctions = ['lv_obj_create', 'lv_obj_set_pos'];
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_button_create(obj);  // This should fail
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('not allowed'));
    }
});

test('Success: LVGL function in allowed list', () => {
    const allowedFunctions = ['lv_obj_create', 'lv_obj_set_pos'];
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 10, 20);
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'obj');
    assertEquals(result.x, 10);
    assertEquals(result.y, 20);
});

test('Error: wrong number of arguments (too few)', () => {
    const allowedFunctions = {
        'lv_obj_create': 1,
        'lv_obj_set_pos': 3
    };
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 10);  // Missing 3rd argument
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('expects 3 argument'));
    }
});

test('Error: wrong number of arguments (too many)', () => {
    const allowedFunctions = {
        'lv_obj_create': 1,
        'lv_obj_set_pos': 3
    };
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0, 1, 2);  // Too many arguments
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('expects 1 argument'));
    }
});

test('Success: correct number of arguments', () => {
    const allowedFunctions = {
        'lv_obj_create': 1,
        'lv_obj_set_pos': 3,
        'lv_obj_set_size': 3
    };
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 100, 200);
            lv_obj_set_size(obj, 50, 60);
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'obj');
    assertEquals(result.x, 100);
    assertEquals(result.y, 200);
    assertEquals(result.width, 50);
    assertEquals(result.height, 60);
});

test('Success: flexible argument count with min/max', () => {
    const allowedFunctions = {
        'lv_obj_create': 1,
        'lv_obj_set_pos': { min: 2, max: 3 }  // Allow 2 or 3 args
    };
    const script = compile_to_javascript(`
        function test() {
            let obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 10, 20);  // 3 args total (obj + 2)
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'obj');
    assertEquals(result.x, 10);
    assertEquals(result.y, 20);
});

// ============================================================================
// TYPE CHECKING TESTS
// ============================================================================

console.log('\n--- Type Checking Tests ---');

test('Type: parse function with typed parameters', () => {
    const code = 'function test(x: number, y: string): bool { return true; }';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].params.length, 2);
    assertEquals(ast.body[0].params[0].name, 'x');
    assertEquals(ast.body[0].params[0].type, 'number');
    assertEquals(ast.body[0].params[1].type, 'string');
    assertEquals(ast.body[0].returnType, 'bool');
});

test('Type: parse variable with type annotation', () => {
    const code = 'let x: number = 42;';
    const lexer = new Lexer(code);
    const parser = new Parser(lexer.tokenize());
    const ast = parser.parseProgram();
    
    assertEquals(ast.body[0].varType, 'number');
});

test('Type: variable type checking success', () => {
    const script = compile_to_javascript(`
        function test() {
            let x: number = 42;
            let y: string = "hello";
            let z: bool = true;
            return x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 42);
});

test('Type: variable type mismatch error', () => {
    const script = compile_to_javascript(`
        function test() {
            let x: number = "hello";
            return x;
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('Type mismatch'));
    }
});

test('Type: function parameter type checking', () => {
    const script = compile_to_javascript(`
        function add(x: number, y: number): number {
            return x + y;
        }
        function test() {
            return add(5, 10);
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('test');
    assertEquals(result, 15);
});

test('Type: function parameter type mismatch', () => {
    const script = compile_to_javascript(`
        function greet(name: string): string {
            return name;
        }
        function test() {
            return greet(123);
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('expects type string'));
    }
});

test('Type: function return type checking', () => {
    const script = compile_to_javascript(`
        function getString(): string {
            return "hello";
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    const result = script.exec('getString');
    assertEquals(result, "hello");
});

test('Type: function return type mismatch', () => {
    const script = compile_to_javascript(`
        function getNumber(): number {
            return "not a number";
        }
    `);
    script.init({}, mockLvgl, mockConstants);
    try {
        script.exec('getNumber');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('return type mismatch'));
    }
});

test('Type: LVGL function with type checking', () => {
    const allowedFunctions = {
        'lv_obj_create': { params: ['number'], returnType: 'lv_obj' },
        'lv_obj_set_pos': { params: ['lv_obj', 'number', 'number'], returnType: 'number' }
    };
    const script = compile_to_javascript(`
        function test(): lv_obj {
            let obj: lv_obj = lv_obj_create(0);
            lv_obj_set_pos(obj, 10, 20);
            return obj;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'obj');
});

test('Type: LVGL function parameter type mismatch', () => {
    const allowedFunctions = {
        'lv_obj_create': { params: ['number'], returnType: 'lv_obj' },
        'lv_obj_set_pos': { params: ['lv_obj', 'number', 'number'], returnType: 'number' }
    };
    const script = compile_to_javascript(`
        function test() {
            lv_obj_set_pos("not an object", 10, 20);
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('expects type lv_obj'));
    }
});

test('Type: custom lv_obj type compatibility', () => {
    const allowedFunctions = {
        'lv_obj_create': { params: ['number'], returnType: 'lv_obj' },
        'lv_button_create': { params: ['lv_obj'], returnType: 'lv_obj' },
        'lv_label_create': { params: ['lv_obj'], returnType: 'lv_obj' }
    };
    const script = compile_to_javascript(`
        function test() {
            let obj: lv_obj = lv_obj_create(0);
            let btn: lv_obj = lv_button_create(obj);
            let lbl: lv_obj = lv_label_create(btn);
            return lbl;
        }
    `);
    script.init({}, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'label');
});

// ============================================================================
// Global Function Type Checking Tests
// ============================================================================

test('Global function type checking: correct types', () => {
    const globals = {
        add: {
            function: (a, b) => a + b,
            params: ['number', 'number'],
            returnType: 'number'
        },
        concat: {
            function: (a, b) => a + b,
            params: ['string', 'string'],
            returnType: 'string'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            let sum = add(5, 3);
            let text = concat("Hello", " World");
            return sum;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 8);
});

test('Global function type checking: wrong parameter type', () => {
    const globals = {
        add: {
            function: (a, b) => a + b,
            params: ['number', 'number'],
            returnType: 'number'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            return add("5", 3);  // String instead of number
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('parameter 1 expects type number, but got string'));
    }
});

test('Global function type checking: wrong argument count', () => {
    const globals = {
        add: {
            function: (a, b) => a + b,
            params: ['number', 'number'],
            returnType: 'number'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            return add(5);  // Missing second argument
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('expects 2 argument'));
    }
});

test('Global function type checking: wrong return type', () => {
    const globals = {
        getString: {
            function: () => "text",
            params: [],
            returnType: 'number'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            return getString();
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('return type mismatch: expected number, but got string'));
    }
});

test('Global function type checking: lv_obj type', () => {
    const globals = {
        createWidget: {
            function: () => ({ type: 'button' }),
            params: [],
            returnType: 'lv_obj'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            let widget = createWidget();
            return widget;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result.type, 'button');
});

test('Nested global function with type checking', () => {
    const globals = {
        System: {
            stringToUTF8: {
                function: (str) => 12345,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const script = compile_to_javascript(`
        function test() {
            let addr = System.stringToUTF8("Hello");
            return addr;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 12345);
});

test('Nested global function type checking: wrong parameter type', () => {
    const globals = {
        System: {
            stringToUTF8: {
                function: (str) => 12345,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const script = compile_to_javascript(`
        function test() {
            return System.stringToUTF8(123);  // Number instead of string
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('parameter 1 expects type string, but got number'));
    }
});

test('Global function with no type specification still works', () => {
    const globals = {
        doSomething: (x) => x * 2
    };
    const script = compile_to_javascript(`
        function test() {
            return doSomething(21);
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 42);
});

// ============================================================================
// CString Type Tests
// ============================================================================

test('CString: variable declaration with string auto-conversion', () => {
    const globals = {
        System: {
            stringToNewUTF8: {
                function: (str) => 99999,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const script = compile_to_javascript(`
        function test() {
            let str: cstring = "Hello World";
            return str;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 99999);
});

test('CString: function parameter auto-conversion', () => {
    const globals = {
        System: {
            stringToNewUTF8: {
                function: (str) => 12345,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const allowedFunctions = {
        'lv_label_create': { params: ['number'], returnType: 'lv_obj' },
        'lv_label_set_text': { params: ['lv_obj', 'cstring'], returnType: 'number' }
    };
    const script = compile_to_javascript(`
        function test() {
            let label: lv_obj = lv_label_create(0);
            lv_label_set_text(label, "My Label");
            return label;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, allowedFunctions);
    const result = script.exec('test');
    assertEquals(result.type, 'label');
});

test('CString: user function parameter auto-conversion', () => {
    const globals = {
        System: {
            stringToNewUTF8: {
                function: (str) => str.length * 1000,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const script = compile_to_javascript(`
        function setLabel(obj: lv_obj, text: cstring) {
            return text;
        }
        
        function test() {
            let label: lv_obj = lv_label_create(0);
            let addr = setLabel(label, "Test");
            return addr;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 4000); // "Test" has 4 chars
});

test('CString: global function parameter auto-conversion', () => {
    const globals = {
        System: {
            stringToNewUTF8: {
                function: (str) => 55555,
                params: ['string'],
                returnType: 'number'
            }
        },
        setText: {
            function: (addr) => addr,
            params: ['cstring'],
            returnType: 'number'
        }
    };
    const script = compile_to_javascript(`
        function test() {
            return setText("Hello");
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 55555);
});

test('CString: cstring value can be used as number', () => {
    const globals = {
        System: {
            stringToNewUTF8: {
                function: (str) => 42,
                params: ['string'],
                returnType: 'number'
            }
        }
    };
    const script = compile_to_javascript(`
        function test() {
            let str: cstring = "Test";
            let num: number = str;  // cstring is compatible with number
            return num;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    const result = script.exec('test');
    assertEquals(result, 42);
});

test('CString: error when System.stringToNewUTF8 not available', () => {
    const globals = {};
    const script = compile_to_javascript(`
        function test() {
            let str: cstring = "Hello";
            return str;
        }
    `);
    script.init(globals, mockLvgl, mockConstants, null);
    try {
        script.exec('test');
        assert(false, 'Should have thrown an error');
    } catch (error) {
        assert(error.message.includes('System.stringToNewUTF8 is required for cstring conversion'));
    }
});

// ============================================================================
// SUMMARY
// ============================================================================

console.log('\n=== Test Summary ===');
console.log(`Total: ${testCount}`);
console.log(`Passed: ${passCount}`);
console.log(`Failed: ${failCount}`);

if (failCount === 0) {
    console.log('\n✓ All tests passed!');
    process.exit(0);
} else {
    console.log(`\n✗ ${failCount} test(s) failed`);
    process.exit(1);
}
