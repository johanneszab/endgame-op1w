//Locate callers of named imported APIs, and print a call graph one level up.
//Args: <outFile> <apiName> [<apiName> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.*;

public class FindApi extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        SymbolTable st = currentProgram.getSymbolTable();
        for (int i = 1; i < a.length; i++) {
            String name = a[i];
            w.println("=== " + name + " ===");
            SymbolIterator si = st.getSymbolIterator(name, true);
            while (si.hasNext()) {
                Symbol s = si.next();
                w.println("  symbol " + s.getName() + " @ " + s.getAddress()
                          + "  type=" + s.getSymbolType());
                Set<Function> callers = new LinkedHashSet<Function>();
                ReferenceIterator ri = currentProgram.getReferenceManager()
                                        .getReferencesTo(s.getAddress());
                while (ri.hasNext()) {
                    Reference r = ri.next();
                    Function f = getFunctionContaining(r.getFromAddress());
                    if (f == null) { w.println("    ref from " + r.getFromAddress() + " (no func)"); continue; }
                    w.println("    ref from " + f.getName() + " @ " + f.getEntryPoint()
                              + " (site " + r.getFromAddress() + ")"
                              + (f.isThunk() ? " [THUNK]" : ""));
                    callers.add(f);
                }
                for (Function f : callers) {
                    w.println("      -- callers of " + f.getName() + " @ " + f.getEntryPoint() + ":");
                    Set<Function> up = new LinkedHashSet<Function>();
                    ReferenceIterator r2 = currentProgram.getReferenceManager()
                                            .getReferencesTo(f.getEntryPoint());
                    while (r2.hasNext()) {
                        Function g = getFunctionContaining(r2.next().getFromAddress());
                        if (g != null) up.add(g);
                    }
                    for (Function g : up) w.println("         " + g.getName() + " @ " + g.getEntryPoint());
                }
            }
        }
        w.flush(); w.close();
        println("FindApi done -> " + a[0]);
    }
}
