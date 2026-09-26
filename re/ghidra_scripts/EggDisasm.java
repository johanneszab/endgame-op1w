//Dump raw disassembly of the function containing each given address.
//Args: <outFile> <hex addr> [<hex addr> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class EggDisasm extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        for (int i = 1; i < a.length; i++) {
            Address ad = toAddr(a[i].trim());
            Function f = getFunctionContaining(ad);
            if (f == null) { w.println("/* no func at " + a[i] + " */"); continue; }
            w.println("==== " + f.getName() + " @ " + f.getEntryPoint() + " ====");
            InstructionIterator it = currentProgram.getListing()
                    .getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction in = it.next();
                w.println(in.getAddress() + "  " + in.toString());
            }
            w.println();
        }
        w.flush(); w.close();
        println("Disasm done -> " + a[0]);
    }
}
