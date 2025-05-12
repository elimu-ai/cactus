interface Parameter {
    type: string,
    description: string,
    required?: boolean // parameter is optional if not specified
}

interface Tool {
    func: Function,
    description: string,
    parameters: {[key: string]: Parameter},
    required: string[]
}

export class Tools {
    private static tools = new Map<string, Tool>();
    
    static add(
        func: Function, 
        description: string,
        parameters: {[key: string]: Parameter},
      ) {
        this.tools.set(func.name, { 
          func, 
          description,
          parameters,
          required: Object.entries(parameters)
            .filter(([_, param]) => param.required)
            .map(([key, _]) => key)
        });
        return func;
      }
    
    static getSchemas() {
        return Array.from(this.tools.entries()).map(([name, { description, parameters, required }]) => ({
          type: "function",
          function: {
            name,
            description,
            parameters: {
              type: "object",
              properties: parameters,
              required
            }
          }
        }));
      }
    
    static async execute(name: string, args: any) {
        const tool = this.tools.get(name);
        if (!tool) throw new Error(`Tool ${name} not found`);
        return await tool.func(...Object.values(args));
    }
}