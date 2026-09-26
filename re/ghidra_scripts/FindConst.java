//Find every instruction whose scalar operand equals one of the given values.
//Args: <outFile> <hexValue> [<hexValue> ...]
//@category EGG
import java.io.File;
import java.io.PrintWriter;
import java.util.*;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.scalar.Scalar;

public class FindConst extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] a = getScriptArgs();
        PrintWriter w = new PrintWriter(new File(a[0]), "UTF-8");
        Set<Long> want = new LinkedHashSet<Long>();
        for (int i = 1; i < a.length; i++) {
            for (String p : a[i].split(",")) {
                p = p.trim();
                if (p.length() == 0) continue;
                if (p.startsWith("0x") || p.startsWith("0X")) p = p.substring(2);
                want.add(Long.parseLong(p, 16));
            }
        }
        w.println("looking for scalars: " + want);
        InstructionIterator it = currentProgram.getListing().getInstructions(true);
        int hits = 0;
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction ins = it.next();
            for (int op = 0; op < ins.getNumOperands(); op++) {
                Object[] objs = ins.getOpObjects(op);
                for (Object o : objs) {
                    if (o instanceof Scalar) {
                        long v = ((Scalar) o).getUnsignedValue();
                        if (want.contains(v)) {
                            Function f = getFunctionContaining(ins.getAddress());
                            w.println(String.format("0x%x  %s @ %s   %s",
                                v,
                                f == null ? "(no func)" : f.getName(),
                                f == null ? "-" : f.getEntryPoint().toString(),
                                ins.getAddress() + "  " + ins.toString()));
                            hits++;
                        }
                    }
                }
            }
        }
        w.println("total hits: " + hits);
        w.flush(); w.close();
        println("FindConst done -> " + a[0] + " hits=" + hits);
    }
}
