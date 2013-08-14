module Puppet::Parser::Functions
  newfunction(:mkpasswd, :type => :rvalue) do |args|
    salt = args[0]
    password_plaintext = args[1]

    if File::exists? "/usr/bin/mkpasswd"
        out = function_generate(["/usr/bin/mkpasswd", "-m", "md5", "-S", salt, password_plaintext])
        out.chomp!
        return out
    else
        # Caller should ensure `mkpasswd' is installed first.
        # Once `mkpasswd' is installed, the password will be automatically changed
        # to the correct value.
        return "dummy_hash_because_mkpasswd_is_not_installed"
    end
  end
end
