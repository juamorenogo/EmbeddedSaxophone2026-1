
## Creation of user

For regular system usage, a non-root user was created in order to avoid operating the system with full administrative privileges at all times. This improves safety and follows standard Linux practices.

The user created for this system is `sebastian-t113`. To create the user, the following command is used:

```bash
adduser sebastian-t113
```

During this process, the system prompts for a password and optional user information. Once completed, the user is created with a home directory and basic configuration.

To allow administrative operations when needed, the user must be added to the sudo group:

```bash
usermod -aG sudo sebastian-t113
```

## 