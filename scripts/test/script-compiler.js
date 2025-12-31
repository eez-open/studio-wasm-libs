/**
 * Simple script language compiler and interpreter
 * 
 * This provides a controlled execution environment for a simple JavaScript-like language.
 * Only supports: functions, variables (let/const), expressions, function calls, if, for, return
 */

// ============================================================================
// LEXER (Tokenizer)
// ============================================================================

class Lexer {
    constructor(input) {
        this.input = input;
        this.pos = 0;
        this.line = 1;
        this.column = 1;
    }

    peek() {
        return this.input[this.pos];
    }

    advance() {
        const ch = this.input[this.pos++];
        if (ch === '\n') {
            this.line++;
            this.column = 1;
        } else {
            this.column++;
        }
        return ch;
    }

    skipWhitespace() {
        while (this.pos < this.input.length && /\s/.test(this.peek())) {
            this.advance();
        }
    }

    skipLineComment() {
        while (this.pos < this.input.length && this.peek() !== '\n') {
            this.advance();
        }
    }

    skipBlockComment() {
        this.advance(); // skip *
        while (this.pos < this.input.length) {
            if (this.peek() === '*' && this.input[this.pos + 1] === '/') {
                this.advance(); // skip *
                this.advance(); // skip /
                break;
            }
            this.advance();
        }
    }

    readNumber() {
        let num = '';
        while (this.pos < this.input.length && /[0-9.]/.test(this.peek())) {
            num += this.advance();
        }
        return { type: 'NUMBER', value: parseFloat(num) };
    }

    readString(quote) {
        this.advance(); // skip opening quote
        let str = '';
        while (this.pos < this.input.length && this.peek() !== quote) {
            if (this.peek() === '\\') {
                this.advance();
                const next = this.advance();
                switch (next) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case 'r': str += '\r'; break;
                    case '\\': str += '\\'; break;
                    case '"': str += '"'; break;
                    case "'": str += "'"; break;
                    default: str += next;
                }
            } else {
                str += this.advance();
            }
        }
        this.advance(); // skip closing quote
        return { type: 'STRING', value: str };
    }

    readIdentifier() {
        let id = '';
        while (this.pos < this.input.length && /[a-zA-Z0-9_]/.test(this.peek())) {
            id += this.advance();
        }

        // Check for keywords
        const keywords = {
            'function': 'FUNCTION',
            'return': 'RETURN',
            'if': 'IF',
            'else': 'ELSE',
            'for': 'FOR',
            'while': 'WHILE',
            'let': 'LET',
            'const': 'CONST',
            'true': 'TRUE',
            'false': 'FALSE',
            'null': 'NULL',
            'undefined': 'UNDEFINED',
            'number': 'TYPE_NUMBER',
            'bool': 'TYPE_BOOL',
            'string': 'TYPE_STRING',
            'cstring': 'TYPE_CSTRING'
        };

        return { type: keywords[id] || 'IDENTIFIER', value: id };
    }

    nextToken() {
        this.skipWhitespace();

        if (this.pos >= this.input.length) {
            return { type: 'EOF' };
        }

        const ch = this.peek();

        // Comments
        if (ch === '/' && this.input[this.pos + 1] === '/') {
            this.skipLineComment();
            return this.nextToken();
        }

        if (ch === '/' && this.input[this.pos + 1] === '*') {
            this.advance(); // skip /
            this.skipBlockComment();
            return this.nextToken();
        }

        // Numbers
        if (/[0-9]/.test(ch)) {
            return this.readNumber();
        }

        // Strings
        if (ch === '"' || ch === "'") {
            return this.readString(ch);
        }

        // Identifiers and keywords
        if (/[a-zA-Z_]/.test(ch)) {
            return this.readIdentifier();
        }

        // Operators and punctuation
        const doubleChar = ch + this.input[this.pos + 1];
        const operators2 = ['==', '!=', '<=', '>=', '&&', '||', '++', '--', '+=', '-=', '*=', '/='];
        if (operators2.includes(doubleChar)) {
            this.advance();
            this.advance();
            return { type: doubleChar, value: doubleChar };
        }

        const singleChar = {
            '(': 'LPAREN', ')': 'RPAREN',
            '{': 'LBRACE', '}': 'RBRACE',
            '[': 'LBRACKET', ']': 'RBRACKET',
            ';': 'SEMICOLON', ',': 'COMMA',
            '.': 'DOT', ':': 'COLON',
            '=': 'ASSIGN',
            '+': 'PLUS', '-': 'MINUS',
            '*': 'STAR', '/': 'SLASH',
            '%': 'PERCENT',
            '<': 'LT', '>': 'GT',
            '!': 'NOT',
            '&': 'AMP', '|': 'PIPE', '^': 'CARET'
        };

        if (singleChar[ch]) {
            this.advance();
            return { type: singleChar[ch], value: ch };
        }

        throw new Error(`Unexpected character '${ch}' at line ${this.line}:${this.column}`);
    }

    tokenize() {
        const tokens = [];
        let token;
        while ((token = this.nextToken()).type !== 'EOF') {
            tokens.push(token);
        }
        tokens.push(token); // Add EOF
        return tokens;
    }
}

// ============================================================================
// PARSER (AST Builder)
// ============================================================================

class Parser {
    constructor(tokens) {
        this.tokens = tokens;
        this.pos = 0;
    }

    peek() {
        return this.tokens[this.pos];
    }

    advance() {
        return this.tokens[this.pos++];
    }

    expect(type) {
        const token = this.advance();
        if (token.type !== type) {
            throw new Error(`Expected ${type} but got ${token.type}`);
        }
        return token;
    }

    // Program = Statement*
    parseProgram() {
        const statements = [];
        while (this.peek().type !== 'EOF') {
            statements.push(this.parseStatement());
        }
        return { type: 'Program', body: statements };
    }

    // Statement = FunctionDeclaration | IfStatement | ForStatement | WhileStatement | ReturnStatement | ExpressionStatement
    parseStatement() {
        const token = this.peek();

        if (token.type === 'FUNCTION') {
            return this.parseFunctionDeclaration();
        }
        if (token.type === 'IF') {
            return this.parseIfStatement();
        }
        if (token.type === 'FOR') {
            return this.parseForStatement();
        }
        if (token.type === 'WHILE') {
            return this.parseWhileStatement();
        }
        if (token.type === 'RETURN') {
            return this.parseReturnStatement();
        }
        if (token.type === 'LET' || token.type === 'CONST') {
            return this.parseVariableDeclaration();
        }
        if (token.type === 'LBRACE') {
            return this.parseBlockStatement();
        }

        return this.parseExpressionStatement();
    }

    // FunctionDeclaration = 'function' Identifier '(' Parameters? ')' (':' Type)? BlockStatement
    parseFunctionDeclaration() {
        this.expect('FUNCTION');
        const name = this.expect('IDENTIFIER').value;
        this.expect('LPAREN');

        const params = [];
        if (this.peek().type !== 'RPAREN') {
            // Parse parameter with optional type annotation
            const paramName = this.expect('IDENTIFIER').value;
            let paramType = null;
            if (this.peek().type === 'COLON') {
                this.advance(); // skip ':'
                paramType = this.parseType();
            }
            params.push({ name: paramName, type: paramType });
            
            while (this.peek().type === 'COMMA') {
                this.advance();
                const pName = this.expect('IDENTIFIER').value;
                let pType = null;
                if (this.peek().type === 'COLON') {
                    this.advance();
                    pType = this.parseType();
                }
                params.push({ name: pName, type: pType });
            }
        }

        this.expect('RPAREN');
        
        // Parse optional return type
        let returnType = null;
        if (this.peek().type === 'COLON') {
            this.advance();
            returnType = this.parseType();
        }
        
        const body = this.parseBlockStatement();

        return { type: 'FunctionDeclaration', name, params, returnType, body };
    }

    // Type = 'number' | 'bool' | 'string' | 'cstring' | Identifier
    parseType() {
        const token = this.advance();
        if (token.type === 'TYPE_NUMBER') {
            return 'number';
        } else if (token.type === 'TYPE_BOOL') {
            return 'bool';
        } else if (token.type === 'TYPE_STRING') {
            return 'string';
        } else if (token.type === 'TYPE_CSTRING') {
            return 'cstring';
        } else if (token.type === 'IDENTIFIER') {
            return token.value; // Custom type like lv_obj
        } else {
            throw new Error(`Expected type but got ${token.type}`);
        }
    }

    // BlockStatement = '{' Statement* '}'
    parseBlockStatement() {
        this.expect('LBRACE');
        const statements = [];
        while (this.peek().type !== 'RBRACE') {
            statements.push(this.parseStatement());
        }
        this.expect('RBRACE');
        return { type: 'BlockStatement', body: statements };
    }

    // VariableDeclaration = ('let' | 'const') Identifier (':' Type)? ('=' Expression)? ';'
    parseVariableDeclaration() {
        const kind = this.advance().type; // LET or CONST
        const name = this.expect('IDENTIFIER').value;
        let varType = null;
        let init = null;

        // Parse optional type annotation
        if (this.peek().type === 'COLON') {
            this.advance();
            varType = this.parseType();
        }

        if (this.peek().type === 'ASSIGN') {
            this.advance();
            init = this.parseExpression();
        }

        if (this.peek().type === 'SEMICOLON') {
            this.advance();
        }

        return { type: 'VariableDeclaration', kind: kind.toLowerCase(), name, varType, init };
    }

    // IfStatement = 'if' '(' Expression ')' Statement ('else' Statement)?
    parseIfStatement() {
        this.expect('IF');
        this.expect('LPAREN');
        const test = this.parseExpression();
        this.expect('RPAREN');
        const consequent = this.parseStatement();
        let alternate = null;

        if (this.peek().type === 'ELSE') {
            this.advance();
            alternate = this.parseStatement();
        }

        return { type: 'IfStatement', test, consequent, alternate };
    }

    // ForStatement = 'for' '(' (VariableDeclaration | ExpressionStatement) Expression ';' Expression ')' Statement
    parseForStatement() {
        this.expect('FOR');
        this.expect('LPAREN');

        let init = null;
        if (this.peek().type === 'LET' || this.peek().type === 'CONST') {
            init = this.parseVariableDeclaration();
        } else if (this.peek().type !== 'SEMICOLON') {
            init = this.parseExpression();
            if (this.peek().type === 'SEMICOLON') {
                this.advance();
            }
        } else {
            this.advance(); // skip semicolon
        }

        const test = this.peek().type !== 'SEMICOLON' ? this.parseExpression() : null;
        this.expect('SEMICOLON');

        const update = this.peek().type !== 'RPAREN' ? this.parseExpression() : null;
        this.expect('RPAREN');

        const body = this.parseStatement();

        return { type: 'ForStatement', init, test, update, body };
    }

    // WhileStatement = 'while' '(' Expression ')' Statement
    parseWhileStatement() {
        this.expect('WHILE');
        this.expect('LPAREN');
        const test = this.parseExpression();
        this.expect('RPAREN');
        const body = this.parseStatement();

        return { type: 'WhileStatement', test, body };
    }

    // ReturnStatement = 'return' Expression? ';'
    parseReturnStatement() {
        this.expect('RETURN');
        let argument = null;

        if (this.peek().type !== 'SEMICOLON' && this.peek().type !== 'RBRACE') {
            argument = this.parseExpression();
        }

        if (this.peek().type === 'SEMICOLON') {
            this.advance();
        }

        return { type: 'ReturnStatement', argument };
    }

    // ExpressionStatement = Expression ';'
    parseExpressionStatement() {
        const expr = this.parseExpression();
        if (this.peek().type === 'SEMICOLON') {
            this.advance();
        }
        return { type: 'ExpressionStatement', expression: expr };
    }

    // Expression = AssignmentExpression
    parseExpression() {
        return this.parseAssignmentExpression();
    }

    // AssignmentExpression = LogicalOrExpression (('=' | '+=' | '-=') AssignmentExpression)?
    parseAssignmentExpression() {
        const left = this.parseLogicalOrExpression();

        if (['ASSIGN', '+=', '-=', '*=', '/='].includes(this.peek().type)) {
            const operator = this.advance().value;
            const right = this.parseAssignmentExpression();
            return { type: 'AssignmentExpression', operator, left, right };
        }

        return left;
    }

    // LogicalOrExpression = LogicalAndExpression ('||' LogicalAndExpression)*
    parseLogicalOrExpression() {
        let left = this.parseLogicalAndExpression();

        while (this.peek().type === '||') {
            const operator = this.advance().value;
            const right = this.parseLogicalAndExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // LogicalAndExpression = BitwiseOrExpression ('&&' BitwiseOrExpression)*
    parseLogicalAndExpression() {
        let left = this.parseBitwiseOrExpression();

        while (this.peek().type === '&&') {
            const operator = this.advance().value;
            const right = this.parseBitwiseOrExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // BitwiseOrExpression = BitwiseXorExpression ('|' BitwiseXorExpression)*
    parseBitwiseOrExpression() {
        let left = this.parseBitwiseXorExpression();

        while (this.peek().type === 'PIPE') {
            const operator = this.advance().value;
            const right = this.parseBitwiseXorExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // BitwiseXorExpression = BitwiseAndExpression ('^' BitwiseAndExpression)*
    parseBitwiseXorExpression() {
        let left = this.parseBitwiseAndExpression();

        while (this.peek().type === 'CARET') {
            const operator = this.advance().value;
            const right = this.parseBitwiseAndExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // BitwiseAndExpression = EqualityExpression ('&' EqualityExpression)*
    parseBitwiseAndExpression() {
        let left = this.parseEqualityExpression();

        while (this.peek().type === 'AMP') {
            const operator = this.advance().value;
            const right = this.parseEqualityExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // EqualityExpression = RelationalExpression (('==' | '!=') RelationalExpression)*
    parseEqualityExpression() {
        let left = this.parseRelationalExpression();

        while (['==', '!='].includes(this.peek().type)) {
            const operator = this.advance().value;
            const right = this.parseRelationalExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // RelationalExpression = AdditiveExpression (('<' | '>' | '<=' | '>=') AdditiveExpression)*
    parseRelationalExpression() {
        let left = this.parseAdditiveExpression();

        while (['LT', 'GT', '<=', '>='].includes(this.peek().type)) {
            const operator = this.advance().value;
            const right = this.parseAdditiveExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // AdditiveExpression = MultiplicativeExpression (('+' | '-') MultiplicativeExpression)*
    parseAdditiveExpression() {
        let left = this.parseMultiplicativeExpression();

        while (['PLUS', 'MINUS'].includes(this.peek().type)) {
            const operator = this.advance().value;
            const right = this.parseMultiplicativeExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // MultiplicativeExpression = UnaryExpression (('*' | '/' | '%') UnaryExpression)*
    parseMultiplicativeExpression() {
        let left = this.parseUnaryExpression();

        while (['STAR', 'SLASH', 'PERCENT'].includes(this.peek().type)) {
            const operator = this.advance().value;
            const right = this.parseUnaryExpression();
            left = { type: 'BinaryExpression', operator, left, right };
        }

        return left;
    }

    // UnaryExpression = ('!' | '-' | '++' | '--') UnaryExpression | PostfixExpression
    parseUnaryExpression() {
        if (['NOT', 'MINUS', '++', '--'].includes(this.peek().type)) {
            const operator = this.advance().value;
            const argument = this.parseUnaryExpression();
            return { type: 'UnaryExpression', operator, prefix: true, argument };
        }

        return this.parsePostfixExpression();
    }

    // PostfixExpression = PrimaryExpression ('(' Arguments? ')' | '.' Identifier | '[' Expression ']' | '++' | '--')*
    parsePostfixExpression() {
        let expr = this.parsePrimaryExpression();

        while (true) {
            const token = this.peek();

            if (token.type === 'LPAREN') {
                // Function call
                this.advance();
                const args = [];
                if (this.peek().type !== 'RPAREN') {
                    args.push(this.parseExpression());
                    while (this.peek().type === 'COMMA') {
                        this.advance();
                        args.push(this.parseExpression());
                    }
                }
                this.expect('RPAREN');
                expr = { type: 'CallExpression', callee: expr, arguments: args };
            } else if (token.type === 'DOT') {
                // Member access
                this.advance();
                const property = this.expect('IDENTIFIER').value;
                expr = { type: 'MemberExpression', object: expr, property, computed: false };
            } else if (token.type === 'LBRACKET') {
                // Computed member access
                this.advance();
                const property = this.parseExpression();
                this.expect('RBRACKET');
                expr = { type: 'MemberExpression', object: expr, property, computed: true };
            } else if (token.type === '++' || token.type === '--') {
                const operator = this.advance().value;
                expr = { type: 'UpdateExpression', operator, prefix: false, argument: expr };
            } else {
                break;
            }
        }

        return expr;
    }

    // PrimaryExpression = Identifier | Literal | '(' Expression ')'
    parsePrimaryExpression() {
        const token = this.peek();

        if (token.type === 'IDENTIFIER') {
            return { type: 'Identifier', name: this.advance().value };
        }

        if (token.type === 'NUMBER') {
            return { type: 'Literal', value: this.advance().value };
        }

        if (token.type === 'STRING') {
            return { type: 'Literal', value: this.advance().value };
        }

        if (token.type === 'TRUE') {
            this.advance();
            return { type: 'Literal', value: true };
        }

        if (token.type === 'FALSE') {
            this.advance();
            return { type: 'Literal', value: false };
        }

        if (token.type === 'NULL') {
            this.advance();
            return { type: 'Literal', value: null };
        }

        if (token.type === 'UNDEFINED') {
            this.advance();
            return { type: 'Literal', value: undefined };
        }

        if (token.type === 'LPAREN') {
            this.advance();
            const expr = this.parseExpression();
            this.expect('RPAREN');
            return expr;
        }

        throw new Error(`Unexpected token ${token.type}`);
    }
}

// ============================================================================
// INTERPRETER (AST Evaluator)
// ============================================================================

class Interpreter {
    constructor(globals, lvgl, constants, allowedFunctions) {
        // Process globals: extract functions and their type specifications
        // globals can be:
        // - plain values/functions: { funcName: function }
        // - functions with types: { funcName: { function: fn, params: [...], returnType: type } }
        this.globalFunctionTypes = {};
        this.globals = this.processGlobalsObject(globals, '');
        
        this.lvgl = lvgl;
        this.constants = constants;
        
        // allowedFunctions can be:
        // - null: allow all functions
        // - array: whitelist of function names
        // - object: { functionName: argCount } for whitelist with arg count validation
        // - object: { functionName: { params: [types], returnType: type } } for full type checking
        this.allowedFunctions = null;
        this.functionArgCounts = {};
        this.functionTypes = {};
        
        if (allowedFunctions) {
            if (Array.isArray(allowedFunctions)) {
                // Array format: just function names
                this.allowedFunctions = allowedFunctions;
            } else if (typeof allowedFunctions === 'object') {
                // Object format with type info
                this.allowedFunctions = Object.keys(allowedFunctions);
                for (const funcName in allowedFunctions) {
                    const spec = allowedFunctions[funcName];
                    if (typeof spec === 'number' || (typeof spec === 'object' && ('min' in spec || 'max' in spec))) {
                        // Old format: just arg count
                        this.functionArgCounts[funcName] = spec;
                    } else if (typeof spec === 'object' && 'params' in spec) {
                        // New format: full type info
                        this.functionTypes[funcName] = spec;
                        this.functionArgCounts[funcName] = spec.params.length;
                    }
                }
            }
        }
        
        this.globalScope = {};
        this.functions = {};
        this.variableTypes = {};
    }

    // Recursively process globals object to extract functions and type info
    processGlobalsObject(obj, pathPrefix) {
        if (!obj) return {};
        
        const result = {};
        for (const key in obj) {
            const value = obj[key];
            const fullPath = pathPrefix ? `${pathPrefix}.${key}` : key;
            
            if (typeof value === 'object' && value !== null && 'function' in value) {
                // Extract function and type info
                result[key] = value.function;
                if (value.params || value.returnType) {
                    this.globalFunctionTypes[fullPath] = {
                        params: value.params || [],
                        returnType: value.returnType
                    };
                }
            } else if (typeof value === 'object' && value !== null && typeof value !== 'function') {
                // Recursively process nested objects
                result[key] = this.processGlobalsObject(value, fullPath);
            } else {
                // Plain value or function
                result[key] = value;
            }
        }
        return result;
    }

    // Build path string from MemberExpression for global function lookup
    getMemberExpressionPath(node) {
        if (node.type === 'Identifier') {
            return node.name;
        } else if (node.type === 'MemberExpression' && !node.computed) {
            const objectPath = this.getMemberExpressionPath(node.object);
            return objectPath ? `${objectPath}.${node.property}` : null;
        }
        return null;
    }

    // Control flow exception for return statements
    createReturnValue(value) {
        return { __return: true, value };
    }

    isReturnValue(value) {
        return value && value.__return === true;
    }

    execute(ast) {
        this.visitProgram(ast);
    }

    visitProgram(node) {
        for (const statement of node.body) {
            this.visitStatement(statement, this.globalScope);
        }
    }

    visitStatement(node, scope) {
        switch (node.type) {
            case 'FunctionDeclaration':
                return this.visitFunctionDeclaration(node, scope);
            case 'VariableDeclaration':
                return this.visitVariableDeclaration(node, scope);
            case 'ExpressionStatement':
                return this.visitExpressionStatement(node, scope);
            case 'BlockStatement':
                return this.visitBlockStatement(node, scope);
            case 'IfStatement':
                return this.visitIfStatement(node, scope);
            case 'ForStatement':
                return this.visitForStatement(node, scope);
            case 'WhileStatement':
                return this.visitWhileStatement(node, scope);
            case 'ReturnStatement':
                return this.visitReturnStatement(node, scope);
            default:
                throw new Error(`Unknown statement type: ${node.type}`);
        }
    }

    visitFunctionDeclaration(node, scope) {
        this.functions[node.name] = node;
        scope[node.name] = (...args) => this.callFunction(node.name, args);
        
        // Store parameter types for this function
        if (node.params && node.params.length > 0 && node.params[0].type) {
            this.variableTypes[node.name] = {
                params: node.params.map(p => p.type),
                returnType: node.returnType
            };
        }
    }

    visitVariableDeclaration(node, scope) {
        let value = node.init ? this.visitExpression(node.init, scope) : undefined;
        
        // Type check if type annotation is present
        if (node.varType && value !== undefined) {
            const actualType = this.getValueType(value);
            
            // Auto-convert string to cstring
            if (node.varType === 'cstring' && actualType === 'string') {
                value = this.convertStringToCString(value);
            } else if (!this.isTypeCompatible(actualType, node.varType)) {
                throw new Error(`Type mismatch: Cannot assign ${actualType} to ${node.varType}`);
            }
        }
        
        scope[node.name] = value;
        
        // Store variable type
        if (node.varType) {
            this.variableTypes[node.name] = node.varType;
        }
    }

    getValueType(value) {
        // Note: cstring is stored as a number (pointer), so we can't distinguish it from number at runtime
        // Type checking for cstring happens at assignment/call time
        if (typeof value === 'number') return 'number';
        if (typeof value === 'boolean') return 'bool';
        if (typeof value === 'string') return 'string';
        if (value === null) return 'null';
        if (value === undefined) return 'undefined';
        // Check if it's an LVGL object (represented as object with type property)
        if (typeof value === 'object' && value !== null && value.type) {
            // Map object types to lv_obj compatible types
            if (value.type === 'obj' || value.type === 'button' || value.type === 'label') {
                return 'lv_obj';
            }
            return value.type;
        }
        return 'unknown';
    }

    isTypeCompatible(actualType, expectedType) {
        if (expectedType === actualType) return true;
        // All lv_* widget types are compatible with lv_obj
        if (expectedType === 'lv_obj' && actualType.startsWith('lv_')) return true;
        // lv_obj is also compatible with number (since it's represented as pointer/number in C)
        if (expectedType === 'lv_obj' && actualType === 'number') return true;
        // cstring is stored as number (pointer) so they're compatible
        if (expectedType === 'cstring' && actualType === 'number') return true;
        if (expectedType === 'number' && actualType === 'cstring') return true;
        return false;
    }

    convertStringToCString(value) {
        // Convert JavaScript string to C string using System.stringToNewUTF8
        if (!this.globals.System || !this.globals.System.stringToNewUTF8) {
            throw new Error('System.stringToNewUTF8 is required for cstring conversion but is not available');
        }
        return this.globals.System.stringToNewUTF8(value);
    }

    visitExpressionStatement(node, scope) {
        return this.visitExpression(node.expression, scope);
    }

    visitBlockStatement(node, scope) {
        const blockScope = Object.create(scope);
        for (const statement of node.body) {
            const result = this.visitStatement(statement, blockScope);
            if (this.isReturnValue(result)) {
                return result;
            }
        }
    }

    visitIfStatement(node, scope) {
        const test = this.visitExpression(node.test, scope);
        if (test) {
            return this.visitStatement(node.consequent, scope);
        } else if (node.alternate) {
            return this.visitStatement(node.alternate, scope);
        }
    }

    visitForStatement(node, scope) {
        const forScope = Object.create(scope);

        if (node.init) {
            if (node.init.type === 'VariableDeclaration') {
                this.visitStatement(node.init, forScope);
            } else {
                this.visitExpression(node.init, forScope);
            }
        }

        while (true) {
            if (node.test) {
                const test = this.visitExpression(node.test, forScope);
                if (!test) break;
            }

            const result = this.visitStatement(node.body, forScope);
            if (this.isReturnValue(result)) {
                return result;
            }

            if (node.update) {
                this.visitExpression(node.update, forScope);
            }
        }
    }

    visitWhileStatement(node, scope) {
        while (true) {
            const test = this.visitExpression(node.test, scope);
            if (!test) break;

            const result = this.visitStatement(node.body, scope);
            if (this.isReturnValue(result)) {
                return result;
            }
        }
    }

    visitReturnStatement(node, scope) {
        const value = node.argument ? this.visitExpression(node.argument, scope) : undefined;
        return this.createReturnValue(value);
    }

    visitExpression(node, scope) {
        switch (node.type) {
            case 'Literal':
                return node.value;
            case 'Identifier':
                return this.visitIdentifier(node, scope);
            case 'BinaryExpression':
                return this.visitBinaryExpression(node, scope);
            case 'UnaryExpression':
                return this.visitUnaryExpression(node, scope);
            case 'UpdateExpression':
                return this.visitUpdateExpression(node, scope);
            case 'AssignmentExpression':
                return this.visitAssignmentExpression(node, scope);
            case 'CallExpression':
                return this.visitCallExpression(node, scope);
            case 'MemberExpression':
                return this.visitMemberExpression(node, scope);
            default:
                throw new Error(`Unknown expression type: ${node.type}`);
        }
    }

    visitIdentifier(node, scope) {
        // Check if it's an LV_* constant
        if (node.name.startsWith('LV_')) {
            if (this.constants && node.name in this.constants) {
                return this.constants[node.name];
            }
            throw new Error(`Unknown constant: ${node.name}`);
        }

        // Check if it's a global object
        if (node.name in this.globals) {
            return this.globals[node.name];
        }

        // Look up in scope chain
        let currentScope = scope;
        while (currentScope) {
            if (node.name in currentScope) {
                return currentScope[node.name];
            }
            currentScope = Object.getPrototypeOf(currentScope);
        }

        throw new Error(`Undefined variable: ${node.name}`);
    }

    visitBinaryExpression(node, scope) {
        const left = this.visitExpression(node.left, scope);
        const right = this.visitExpression(node.right, scope);

        switch (node.operator) {
            case '+': return left + right;
            case '-': return left - right;
            case '*': return left * right;
            case '/': return left / right;
            case '%': return left % right;
            case '==': return left == right;
            case '!=': return left != right;
            case '<': return left < right;
            case '>': return left > right;
            case '<=': return left <= right;
            case '>=': return left >= right;
            case '&&': return left && right;
            case '||': return left || right;
            case '|': return left | right;
            case '&': return left & right;
            case '^': return left ^ right;
            default:
                throw new Error(`Unknown binary operator: ${node.operator}`);
        }
    }

    visitUnaryExpression(node, scope) {
        const argument = this.visitExpression(node.argument, scope);

        switch (node.operator) {
            case '!': return !argument;
            case '-': return -argument;
            case '++':
                if (node.prefix) {
                    return this.incrementVariable(node.argument, scope, 1, true);
                }
                break;
            case '--':
                if (node.prefix) {
                    return this.incrementVariable(node.argument, scope, -1, true);
                }
                break;
            default:
                throw new Error(`Unknown unary operator: ${node.operator}`);
        }
    }

    visitUpdateExpression(node, scope) {
        const delta = node.operator === '++' ? 1 : -1;
        return this.incrementVariable(node.argument, scope, delta, node.prefix);
    }

    incrementVariable(node, scope, delta, prefix) {
        if (node.type !== 'Identifier') {
            throw new Error('Can only increment variables');
        }

        const oldValue = this.visitIdentifier(node, scope);
        const newValue = oldValue + delta;

        // Find the scope that contains this variable and update it
        let currentScope = scope;
        while (currentScope) {
            if (Object.prototype.hasOwnProperty.call(currentScope, node.name)) {
                currentScope[node.name] = newValue;
                return prefix ? newValue : oldValue;
            }
            currentScope = Object.getPrototypeOf(currentScope);
        }

        // If not found in any scope, this shouldn't happen since visitIdentifier succeeded
        throw new Error(`Cannot increment undefined variable: ${node.name}`);
    }

    visitAssignmentExpression(node, scope) {
        const value = this.visitExpression(node.right, scope);
        
        if (node.left.type === 'Identifier') {
            const name = node.left.name;
            
            if (node.operator === '=') {
                // Find the scope that contains this variable and update it
                let currentScope = scope;
                while (currentScope) {
                    if (Object.prototype.hasOwnProperty.call(currentScope, name)) {
                        currentScope[name] = value;
                        return value;
                    }
                    currentScope = Object.getPrototypeOf(currentScope);
                }
                throw new Error(`Cannot assign to undefined variable: ${name}`);
            } else {
                // Compound assignment
                const oldValue = this.visitIdentifier(node.left, scope);
                let newValue;
                switch (node.operator) {
                    case '+=': newValue = oldValue + value; break;
                    case '-=': newValue = oldValue - value; break;
                    case '*=': newValue = oldValue * value; break;
                    case '/=': newValue = oldValue / value; break;
                    default: throw new Error(`Unknown assignment operator: ${node.operator}`);
                }
                
                let currentScope = scope;
                while (currentScope) {
                    if (Object.prototype.hasOwnProperty.call(currentScope, name)) {
                        currentScope[name] = newValue;
                        return newValue;
                    }
                    currentScope = Object.getPrototypeOf(currentScope);
                }
                throw new Error(`Cannot assign to undefined variable: ${name}`);
            }
        }
        
        throw new Error('Invalid assignment target');
    }

    visitCallExpression(node, scope) {
        // Evaluate callee
        let func;
        let thisContext = null;
        let functionName = null;
        let isGlobalFunction = false;

        if (node.callee.type === 'Identifier') {
            const name = node.callee.name;
            functionName = name;

            // Check if it's an lv_* function
            if (name.startsWith('lv_')) {
                // Check if function is in allowed list (if list is provided)
                if (this.allowedFunctions !== null && !this.allowedFunctions.includes(name)) {
                    throw new Error(`LVGL function not allowed: ${name}`);
                }
                
                const lvglFuncName = '_' + name;
                if (this.lvgl && typeof this.lvgl[lvglFuncName] === 'function') {
                    func = this.lvgl[lvglFuncName].bind(this.lvgl);
                } else {
                    throw new Error(`Unknown LVGL function: ${name}`);
                }
            } else {
                func = this.visitIdentifier(node.callee, scope);
                // Check if this is a global function
                if (name in this.globals && typeof this.globals[name] === 'function') {
                    isGlobalFunction = true;
                    functionName = name;
                }
            }
        } else if (node.callee.type === 'MemberExpression') {
            thisContext = this.visitExpression(node.callee.object, scope);
            const propName = node.callee.computed 
                ? this.visitExpression(node.callee.property, scope)
                : node.callee.property;
            func = thisContext[propName];
            
            // Build full path for global function type checking
            if (node.callee.type === 'MemberExpression' && !node.callee.computed) {
                const path = this.getMemberExpressionPath(node.callee);
                if (path && path in this.globalFunctionTypes) {
                    isGlobalFunction = true;
                    functionName = path;
                }
            }
        } else {
            func = this.visitExpression(node.callee, scope);
        }

        // Evaluate arguments
        const args = node.arguments.map(arg => this.visitExpression(arg, scope));

        // Validate types for global functions if specified
        if (isGlobalFunction && functionName in this.globalFunctionTypes) {
            const typeSpec = this.globalFunctionTypes[functionName];
            
            // Check argument count
            if (args.length !== typeSpec.params.length) {
                throw new Error(`Function ${functionName} expects ${typeSpec.params.length} argument(s), but got ${args.length}`);
            }
            
            // Check argument types and perform auto-conversion
            for (let i = 0; i < typeSpec.params.length; i++) {
                const expectedType = typeSpec.params[i];
                const actualType = this.getValueType(args[i]);
                
                // Auto-convert string to cstring
                if (expectedType === 'cstring' && actualType === 'string') {
                    args[i] = this.convertStringToCString(args[i]);
                } else if (!this.isTypeCompatible(actualType, expectedType)) {
                    throw new Error(`Function ${functionName} parameter ${i + 1} expects type ${expectedType}, but got ${actualType}`);
                }
            }
        }

        // Validate argument count and types for lv_* functions if specified
        if (functionName && functionName.startsWith('lv_')) {
            // Check argument count
            if (functionName in this.functionArgCounts) {
                const expectedCount = this.functionArgCounts[functionName];
                if (typeof expectedCount === 'number') {
                    if (args.length !== expectedCount) {
                        throw new Error(`Function ${functionName} expects ${expectedCount} argument(s), but got ${args.length}`);
                    }
                } else if (typeof expectedCount === 'object' && ('min' in expectedCount || 'max' in expectedCount)) {
                    const min = expectedCount.min || 0;
                    const max = expectedCount.max || Infinity;
                    if (args.length < min || args.length > max) {
                        throw new Error(`Function ${functionName} expects ${min}-${max} arguments, but got ${args.length}`);
                    }
                }
            }
            
            // Check argument types
            if (functionName in this.functionTypes) {
                const typeSpec = this.functionTypes[functionName];
                for (let i = 0; i < typeSpec.params.length; i++) {
                    const expectedType = typeSpec.params[i];
                    const actualType = this.getValueType(args[i]);
                    
                    // Auto-convert string to cstring
                    if (expectedType === 'cstring' && actualType === 'string') {
                        args[i] = this.convertStringToCString(args[i]);
                    } else if (!this.isTypeCompatible(actualType, expectedType)) {
                        throw new Error(`Function ${functionName} parameter ${i + 1} expects type ${expectedType}, but got ${actualType}`);
                    }
                }
            }
        }

        // Call function
        if (typeof func !== 'function') {
            throw new Error(`Cannot call non-function: ${typeof func}`);
        }

        const returnValue = func.apply(thisContext, args);
        
        // Validate return type for global functions if specified
        if (isGlobalFunction && functionName in this.globalFunctionTypes) {
            const typeSpec = this.globalFunctionTypes[functionName];
            if (typeSpec.returnType && returnValue !== undefined) {
                const actualType = this.getValueType(returnValue);
                if (!this.isTypeCompatible(actualType, typeSpec.returnType)) {
                    throw new Error(`Function ${functionName} return type mismatch: expected ${typeSpec.returnType}, but got ${actualType}`);
                }
            }
        }
        
        return returnValue;
    }

    visitMemberExpression(node, scope) {
        const object = this.visitExpression(node.object, scope);
        const property = node.computed 
            ? this.visitExpression(node.property, scope)
            : node.property;

        return object[property];
    }

    callFunction(name, args) {
        const func = this.functions[name];
        if (!func) {
            throw new Error(`Function not found: ${name}`);
        }

        // Create new scope for function execution
        const funcScope = Object.create(this.globalScope);

        // Bind parameters with type checking
        for (let i = 0; i < func.params.length; i++) {
            const param = func.params[i];
            const paramName = typeof param === 'string' ? param : param.name;
            const paramType = typeof param === 'object' ? param.type : null;
            
            // Type check parameter if type is specified
            if (paramType && args[i] !== undefined) {
                const actualType = this.getValueType(args[i]);
                
                // Auto-convert string to cstring
                if (paramType === 'cstring' && actualType === 'string') {
                    args[i] = this.convertStringToCString(args[i]);
                } else if (!this.isTypeCompatible(actualType, paramType)) {
                    throw new Error(`Function ${name} parameter ${i + 1} expects type ${paramType}, but got ${actualType}`);
                }
            }
            
            funcScope[paramName] = args[i];
        }

        // Execute function body
        const result = this.visitBlockStatement(func.body, funcScope);

        // Return value with type checking
        const returnValue = this.isReturnValue(result) ? result.value : undefined;
        
        if (func.returnType && returnValue !== undefined) {
            const actualType = this.getValueType(returnValue);
            if (!this.isTypeCompatible(actualType, func.returnType)) {
                throw new Error(`Function ${name} return type mismatch: expected ${func.returnType}, but got ${actualType}`);
            }
        }
        
        return returnValue;
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

function compile_to_javascript(script) {
    let ast;
    
    try {
        // Tokenize
        const lexer = new Lexer(script);
        const tokens = lexer.tokenize();
        
        // Parse
        const parser = new Parser(tokens);
        ast = parser.parseProgram();
    } catch (error) {
        throw new Error(`Parse error: ${error.message}`);
    }

    return {
        _ast: ast,
        _interpreter: null,

        // Initialize with globals, lvgl instance, constants, and optional allowed functions list
        // globals can include type specifications: { funcName: { function: fn, params: [...], returnType: type } }
        init: function(globals, lvgl, constants, allowedFunctions) {
            this._interpreter = new Interpreter(globals, lvgl, constants, allowedFunctions);
            this._interpreter.execute(this._ast);
        },

        // Execute a function by name with arguments
        exec: function(functionName, ...args) {
            if (!this._interpreter) {
                throw new Error('Script not initialized. Call init(globals, lvgl, LVGL_CONSTANTS, allowedFunctions) first.');
            }

            return this._interpreter.callFunction(functionName, args);
        }
    };
}

// Export for use in browser or Node.js
if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
    module.exports = { 
        compile_to_javascript,
        Lexer,
        Parser,
        Interpreter
    };
}
